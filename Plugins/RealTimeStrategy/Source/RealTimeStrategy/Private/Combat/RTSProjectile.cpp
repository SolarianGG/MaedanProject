#include "Combat/RTSProjectile.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundCue.h"

#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "Combat/RTSProjectileTargetComponent.h"
#include "Libraries/RTSCollisionLibrary.h"


ARTSProjectile::ARTSProjectile(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->InitialSpeed = 1000.0f;

    bFired = false;
    bImpactHandled = false;
    HomingHitRadius = 50.0f;

	// Enable replication.
	// This might change in the future, as we don't really care about exact projectile positions on client-side.
	bReplicates = true;

    // Set reasonable default values.
    BallisticTrajectoryFactor = 600.0f;

    AreaOfEffect = 1000.0f;
    AreaOfEffectTargetObjectTypeFilter.Add(EObjectTypeQuery::ObjectTypeQuery2); // WorldDynamic
    AreaOfEffectTargetObjectTypeFilter.Add(EObjectTypeQuery::ObjectTypeQuery3); // Pawn
    AreaOfEffectTargetClassFilter = APawn::StaticClass();
}

void ARTSProjectile::FireAt(
	AActor* ProjectileTarget,
	float ProjectileDamage,
	TSubclassOf<class UDamageType> ProjectileDamageType,
	AController* ProjectileEventInstigator,
	AActor* ProjectileDamageCauser)
{
	if (!ProjectileTarget)
	{
		UE_LOG(LogRTS, Error, TEXT("No target set for projectile %s!"), *GetName());
		return;
	}

	MulticastFireAt(ProjectileTarget, ProjectileDamage, ProjectileDamageType, ProjectileEventInstigator, ProjectileDamageCauser);
}

void ARTSProjectile::Tick(float DeltaSeconds)
{
    if (!bFired)
    {
        return;
    }

    if (ProjectileMovement->bIsHomingProjectile)
    {
        // Only the server runs authoritative hit detection.
        if (!HasAuthority())
        {
            return;
        }

        // Target died before projectile arrived — destroy silently.
        if (!IsValid(Target))
        {
            Destroy();
            return;
        }

        // Check distance to the edge of the target's collision, not to its center.
        float TargetRadius = 0.0f;
        UCapsuleComponent* TargetCapsule = Target->FindComponentByClass<UCapsuleComponent>();
        if (TargetCapsule)
        {
            TargetRadius = TargetCapsule->GetScaledCapsuleRadius();
        }
        const float EffectiveRadius = HomingHitRadius + TargetRadius;
        const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
        if (DistSq > EffectiveRadius * EffectiveRadius)
        {
            return;
        }
    }
    else
    {
        TimeToImpact -= DeltaSeconds;

        // Update ballistic trajectory.
        if (bBallisticTrajectory)
        {
            static const float G = 9.8067f;

            float InitialTravelTime = InitialDistance / ProjectileMovement->InitialSpeed;
            float PassedTravelTime = InitialTravelTime - TimeToImpact;
            float TraveledDistance = PassedTravelTime * ProjectileMovement->InitialSpeed;

            float ProjectileHeight = TraveledDistance * FMath::Tan(LaunchAngle) -
                ((G * (TraveledDistance * TraveledDistance)) /
                (2 * FMath::Pow(ProjectileMovement->InitialSpeed * FMath::Cos(LaunchAngle), 2)));

            FVector ProjectileLocation = GetActorLocation();
            ProjectileLocation.Z = InitialHeight + (ProjectileHeight * BallisticTrajectoryFactor) +
                ((TargetHeight - InitialHeight) * (PassedTravelTime / InitialTravelTime));
            SetActorLocation(ProjectileLocation);
        }

        if (TimeToImpact > 0.0f)
        {
            return;
        }
    }

    if (bImpactHandled)
    {
        Destroy();
        return;
    }

    if (HasAuthority())
    {
        bImpactHandled = true;

        if (!bApplyAreaOfEffect)
        {
            HitTargetActor(Target);
        }
        else
        {
            HitTargetLocation();
        }

        // Reliable multicast is queued before Destroy() replication, so clients receive it first.
        MulticastNotifyHit(Target, Damage, DamageType, EventInstigator, DamageCauser);
    }

    // Destroy projectile. Server destroys; clients follow via replication.
    Destroy();
}

void ARTSProjectile::NotifyOnProjectileDetonated(
	AActor* ProjectileTarget,
	float ProjectileDamage,
	TSubclassOf<class UDamageType> ProjectileDamageType,
	AController* ProjectileEventInstigator,
	AActor* ProjectileDamageCauser)
{
	ReceiveOnProjectileDetonated(ProjectileTarget, ProjectileDamage, ProjectileDamageType, ProjectileEventInstigator, ProjectileDamageCauser);
}

void ARTSProjectile::HitTargetActor(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    UE_LOG(LogRTS, Log, TEXT("Projectile %s hit target %s for %f damage."), *GetName(), *Actor->GetName(), Damage);

    // Deal damage.
    Actor->TakeDamage(Damage, FDamageEvent(DamageType), EventInstigator, DamageCauser);
}

void ARTSProjectile::HitTargetLocation()
{
    // Overlap actors in target area.
    TArray<AActor*> OverlapedActors;
    TArray<AActor*> ActorsToIgnore;

    UKismetSystemLibrary::CapsuleOverlapActors(this, FVector(TargetLocation.X, TargetLocation.Y, 0.0f),
        AreaOfEffect, 10000.0f,
        AreaOfEffectTargetObjectTypeFilter, AreaOfEffectTargetClassFilter,
        ActorsToIgnore, OverlapedActors);

    // Collect valid targets (e.g. by owner).
    for (AActor* OverlapedActor : OverlapedActors)
    {
        if (!IsValid(OverlapedActor))
        {
            continue;
        }

        // Note that we always apply the effects to the real projectile target.
        // This is necessary for forced attacks to friendly units.
        if (OverlapedActor == Target)
        {
            HitTargetActor(OverlapedActor);
            continue;
        }

        // Check owner.
        URTSOwnerComponent* OwnerComponent = OverlapedActor->FindComponentByClass<URTSOwnerComponent>();

        if (IsValid(OwnerComponent) && !OwnerComponent->IsSameTeamAsActor(DamageCauser))
        {
            HitTargetActor(OverlapedActor);
            continue;
        }
    }
}

void ARTSProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    if (!HasAuthority() || !bFired || bImpactHandled)
    {
        return;
    }

    bool bShouldHit = bApplyAreaOfEffect ? IsValid(OtherActor) : (OtherActor == Target);
    if (!bShouldHit)
    {
        return;
    }

    bImpactHandled = true;

    if (!bApplyAreaOfEffect)
    {
        HitTargetActor(Target);
    }
    else
    {
        HitTargetLocation();
    }

    MulticastNotifyHit(Target, Damage, DamageType, EventInstigator, DamageCauser);
    Destroy();
}

void ARTSProjectile::MulticastNotifyHit_Implementation(
    AActor* ProjectileTarget,
    float ProjectileDamage,
    TSubclassOf<class UDamageType> ProjectileDamageType,
    AController* ProjectileEventInstigator,
    AActor* ProjectileDamageCauser)
{
    NotifyOnProjectileDetonated(
        ProjectileTarget, ProjectileDamage, ProjectileDamageType,
        ProjectileEventInstigator, ProjectileDamageCauser);

    if (IsValid(ImpactSound))
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation(), GetActorRotation());
    }
}

void ARTSProjectile::MulticastFireAt_Implementation(AActor* ProjectileTarget, float ProjectileDamage,
    TSubclassOf<class UDamageType> ProjectileDamageType, AController* ProjectileEventInstigator,
    AActor* ProjectileDamageCauser)
{
    Target = ProjectileTarget;
    Damage = ProjectileDamage;
    DamageType = ProjectileDamageType;
    EventInstigator = ProjectileEventInstigator;
    DamageCauser = ProjectileDamageCauser;

    // Find target location.
    URTSProjectileTargetComponent* ProjectileTargetComponent =
        Target->FindComponentByClass<URTSProjectileTargetComponent>();

    if (IsValid(ProjectileTargetComponent))
    {
        TargetLocation = ProjectileTargetComponent->GetRandomProjectileTargetLocation();
    }
    else
    {
        TargetLocation = Target->GetActorLocation();
    }

    // Set direction.
    FVector Direction = TargetLocation - GetActorLocation();
    FVector DirectionNormalized = Direction.GetSafeNormal(0.01f);

    InitialDistance = Direction.Size();
    InitialHeight = GetActorLocation().Z;
    TargetHeight = TargetLocation.Z;

    ProjectileMovement->Velocity = DirectionNormalized * ProjectileMovement->InitialSpeed;

    if (ProjectileMovement->bIsHomingProjectile)
    {
        // Set target.
        ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
    }

    // Reduce effective distance by target's capsule radius so the hit fires at collision edge, not center.
    float TargetCapsuleRadius = 0.0f;
    UCapsuleComponent* TargetCapsule = Target->FindComponentByClass<UCapsuleComponent>();
    if (TargetCapsule)
    {
        TargetCapsuleRadius = TargetCapsule->GetScaledCapsuleRadius();
    }
    TimeToImpact = FMath::Max(0.f, InitialDistance - TargetCapsuleRadius) / ProjectileMovement->InitialSpeed;
    bFired = true;
    bImpactHandled = false;

    // Setup ballistic trajectory.
    if (bBallisticTrajectory)
    {
        // Calculate angle of reach.
        static const float G = 9.8067f;
        LaunchAngle = 0.5f * FMath::Asin(G * InitialDistance / (ProjectileMovement->InitialSpeed * ProjectileMovement->InitialSpeed));
    }

    // Play sound.
    if (IsValid(FiredSound))
    {
        UGameplayStatics::PlaySoundAtLocation(this, FiredSound, GetActorLocation(), GetActorRotation());
    }

    // Stop position replication (clients simulate locally) but keep the actor channel open
    // so MulticastNotifyHit can reach clients when the projectile detonates.
    SetReplicateMovement(false);
}
