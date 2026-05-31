#include "Combat/RTSAttackComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "RTSLog.h"
#include "RTSPlayerAdvantageComponent.h"
#include "Animation/AnimInstance.h"
#include "Combat/RTSProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Libraries/RTSGameplayTagLibrary.h"


URTSAttackComponent::URTSAttackComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// Set reasonable default values.
	AcquisitionRadius = 1000.0f;
	ChaseRadius = 1000.0f;

	FRTSAttackData DefaultAttack;
	DefaultAttack.Cooldown = 0.5f;
	DefaultAttack.Damage = 10.0f;
	DefaultAttack.Range = 200.0f;

	Attacks.Add(DefaultAttack);

	InitialGameplayTags.AddTag(URTSGameplayTagLibrary::Status_Permanent_CanAttack());
}

void URTSAttackComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	// Update cooldown timer.
	if (RemainingCooldown > 0)
	{
		RemainingCooldown -= DeltaTime;

		if (RemainingCooldown <= 0)
		{
			UE_LOG(LogRTS, Log, TEXT("Actor %s attack is ready again."), *GetOwner()->GetName());

			// Notify listeners.
			OnCooldownReady.Broadcast(GetOwner());
		}
	}

	// On the server: cancel attack montage and pending projectile if the pawn starts moving.
	AActor* Owner = GetOwner();
	if (CurrentAttackMontage && Owner->HasAuthority())
	{
		USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		const bool bMontageStillPlaying = Mesh && Mesh->GetAnimInstance()
			&& Mesh->GetAnimInstance()->Montage_IsPlaying(CurrentAttackMontage);

		if (bMontageStillPlaying)
		{
			APawn* Pawn = Cast<APawn>(Owner);
			if (Pawn && Pawn->GetVelocity().SizeSquared() > 1.0f)
			{
				PendingProjectile.Target.Reset();
				CurrentAttackMontage = nullptr;
				MulticastStopAttackMontage();
			}
		}
		else
		{
			// Montage finished naturally.
			CurrentAttackMontage = nullptr;
		}
	}
}

void URTSAttackComponent::UseAttack(int32 AttackIndex, AActor* Target)
{
	AActor* Owner = GetOwner();

	// Only the server should execute attacks; multicast RPCs only propagate from authority.
	if (!Owner->HasAuthority())
	{
		return;
	}
	APawn* OwnerPawn = Cast<APawn>(Owner);
	AController* OwnerController = Cast<AController>(Owner->GetOwner());

	if (!IsValid(Target))
	{
		return;
	}

	// Check cooldown.
	if (RemainingCooldown > 0)
	{
		return;
	}

	// Calculate damage.
	if (!Attacks.IsValidIndex(AttackIndex))
	{
		return;
	}

	const FRTSAttackData& Attack = Attacks[AttackIndex];

	float Damage = Attack.Damage;

	if (IsValid(OwnerController))
	{
		URTSPlayerAdvantageComponent* PlayerAdvantageComponent = OwnerController->FindComponentByClass<
			URTSPlayerAdvantageComponent>();

		if (IsValid(PlayerAdvantageComponent))
		{
			Damage *= PlayerAdvantageComponent->GetOutgoingDamageFactor();
		}
	}

	// Use attack.
	UE_LOG(LogRTS, Log, TEXT("Actor %s attacks %s."), *Owner->GetName(), *Target->GetName());

	// Calculate montage PlayRate, capping if MaxPlayRate is set.
	bool bPlayRateCapped = false;
	if (Attack.AttackMontage)
	{
		float PlayRate = 1.0f;
		if (Attack.Cooldown > 0.0f)
		{
			PlayRate = Attack.AttackMontage->GetPlayLength() / Attack.Cooldown;
		}
		if (Attack.MaxPlayRate > 0.0f && PlayRate > Attack.MaxPlayRate)
		{
			PlayRate = Attack.MaxPlayRate;
			bPlayRateCapped = true;
		}
		MulticastPlayAttackMontage(Attack.AttackMontage, PlayRate);
	}

	// When PlayRate is capped the animation outlasts the cooldown, so the next UseAttack call will
	// interrupt the current montage before AnimNotify fires. In that case damage is applied immediately.
	const bool bUseAnimNotify = Attack.bWaitForAnimNotify && !bPlayRateCapped;

	ARTSProjectile* SpawnedProjectile = nullptr;
	if (Attack.ProjectileClass != nullptr)
	{
		if (bUseAnimNotify)
		{
			// Store data and wait for RTSAnimNotify_FireProjectile in the montage.
			PendingProjectile.Target = Target;
			PendingProjectile.Damage = Damage;
			PendingProjectile.DamageType = Attack.DamageType;
			PendingProjectile.ProjectileClass = Attack.ProjectileClass;
			PendingProjectile.SpawnSocket = Attack.ProjectileSpawnSocket;
			PendingProjectile.Instigator = OwnerController;
		}
		else if (Attack.ProjectileSpawnDelay > 0.0f && !bPlayRateCapped)
		{
			PendingProjectile.Target = Target;
			PendingProjectile.Damage = Damage;
			PendingProjectile.DamageType = Attack.DamageType;
			PendingProjectile.ProjectileClass = Attack.ProjectileClass;
			PendingProjectile.SpawnSocket = Attack.ProjectileSpawnSocket;
			PendingProjectile.Instigator = OwnerController;

			GetWorld()->GetTimerManager().SetTimer(
				ProjectileSpawnTimerHandle,
				this,
				&URTSAttackComponent::SpawnPendingProjectile,
				Attack.ProjectileSpawnDelay,
				false);
		}
		else
		{
			// Fire projectile immediately.
			FVector SpawnLocation = Owner->GetActorLocation();
			FRotator SpawnRotation = Owner->GetActorRotation();

			if (!Attack.ProjectileSpawnSocket.IsNone())
			{
				USkeletalMeshComponent* MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();
				if (MeshComp && MeshComp->DoesSocketExist(Attack.ProjectileSpawnSocket))
				{
					SpawnLocation = MeshComp->GetSocketLocation(Attack.ProjectileSpawnSocket);
					SpawnRotation = MeshComp->GetSocketRotation(Attack.ProjectileSpawnSocket);
				}
			}

			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Instigator = OwnerPawn;
			SpawnInfo.ObjectFlags |= RF_Transient;

			SpawnedProjectile = GetWorld()->SpawnActor<ARTSProjectile>(
				Attack.ProjectileClass, FTransform(SpawnRotation, SpawnLocation), SpawnInfo);

			if (SpawnedProjectile)
			{
				UE_LOG(LogRTS, Log, TEXT("%s fired projectile %s at target %s."), *Owner->GetName(),
				       *SpawnedProjectile->GetName(), *Target->GetName());

				SpawnedProjectile->FireAt(Target, Damage, Attack.DamageType, OwnerController, Owner);
			}
		}
	}
	else
	{
		if (bUseAnimNotify)
		{
			PendingProjectile.Target          = Target;
			PendingProjectile.Damage          = Damage;
			PendingProjectile.DamageType      = Attack.DamageType;
			PendingProjectile.ProjectileClass = nullptr;
			PendingProjectile.SpawnSocket     = NAME_None;
			PendingProjectile.Instigator      = OwnerController;
		}
		else
		{
			Target->TakeDamage(Damage, FDamageEvent(Attack.DamageType), OwnerController, Owner);
		OnAttackLanded.Broadcast(Owner, Target);
		}
	}

	// Start cooldown timer.
	RemainingCooldown = Attack.Cooldown;

	// Notify listeners (server-side, with full data).
	OnAttackUsed.Broadcast(Owner, Attack, Target, SpawnedProjectile);

	// Replicate to clients so they can react (e.g. battle music).
	MulticastNotifyAttackUsed(Owner, Target);
}

float URTSAttackComponent::GetAcquisitionRadius() const
{
	return AcquisitionRadius;
}

float URTSAttackComponent::GetChaseRadius() const
{
	return ChaseRadius;
}

TArray<FRTSAttackData> URTSAttackComponent::GetAttacks() const
{
	return Attacks;
}

float URTSAttackComponent::GetRemainingCooldown() const
{
	return RemainingCooldown;
}

void URTSAttackComponent::MulticastNotifyAttackUsed_Implementation(AActor* InActor, AActor* InTarget)
{
	// On clients, broadcast OnAttackUsed with minimal data (no Attack struct or Projectile).
	if (!GetOwner()->HasAuthority())
	{
		FRTSAttackData DummyAttack;
		OnAttackUsed.Broadcast(InActor, DummyAttack, InTarget, nullptr);
	}
}

void URTSAttackComponent::MulticastPlayAttackMontage_Implementation(UAnimMontage* Montage, float PlayRate)
{
	CurrentAttackMontage = Montage;
	if (auto* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
		if (auto* Anim = Mesh->GetAnimInstance())
			Anim->Montage_Play(Montage, PlayRate);
}

void URTSAttackComponent::MulticastStopAttackMontage_Implementation()
{
	UAnimMontage* Montage = CurrentAttackMontage;
	CurrentAttackMontage = nullptr;
	if (auto* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
		if (auto* Anim = Mesh->GetAnimInstance())
			Anim->Montage_Stop(0.1f, Montage);
}

void URTSAttackComponent::FirePendingProjectile()
{
	SpawnPendingProjectile();
}

void URTSAttackComponent::ApplyPendingMeleeAttack()
{
	if (!PendingProjectile.Target.IsValid())
	{
		return;
	}

	AActor* Owner = GetOwner();

	UE_LOG(LogRTS, Log, TEXT("%s applies pending melee damage to %s."),
		*Owner->GetName(), *PendingProjectile.Target->GetName());

	AActor* HitTarget = PendingProjectile.Target.Get();

	HitTarget->TakeDamage(
		PendingProjectile.Damage,
		FDamageEvent(PendingProjectile.DamageType),
		PendingProjectile.Instigator.Get(),
		Owner);

	OnAttackLanded.Broadcast(Owner, HitTarget);

	PendingProjectile.Target.Reset();
}

void URTSAttackComponent::SpawnPendingProjectile()
{
	if (!PendingProjectile.Target.IsValid())
	{
		return;
	}

	AActor* Owner = GetOwner();
	APawn* OwnerPawn = Cast<APawn>(Owner);

	FVector SpawnLocation = Owner->GetActorLocation();
	FRotator SpawnRotation = Owner->GetActorRotation();

	if (!PendingProjectile.SpawnSocket.IsNone())
	{
		USkeletalMeshComponent* MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();
		if (MeshComp && MeshComp->DoesSocketExist(PendingProjectile.SpawnSocket))
		{
			SpawnLocation = MeshComp->GetSocketLocation(PendingProjectile.SpawnSocket);
			SpawnRotation = MeshComp->GetSocketRotation(PendingProjectile.SpawnSocket);
		}
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = OwnerPawn;
	SpawnInfo.ObjectFlags |= RF_Transient;

	ARTSProjectile* SpawnedProjectile = GetWorld()->SpawnActor<ARTSProjectile>(
		PendingProjectile.ProjectileClass,
		FTransform(SpawnRotation, SpawnLocation),
		SpawnInfo);

	if (SpawnedProjectile)
	{
		UE_LOG(LogRTS, Log, TEXT("%s fired delayed projectile %s at target %s."),
			*Owner->GetName(), *SpawnedProjectile->GetName(), *PendingProjectile.Target->GetName());

		SpawnedProjectile->FireAt(
			PendingProjectile.Target.Get(),
			PendingProjectile.Damage,
			PendingProjectile.DamageType,
			PendingProjectile.Instigator.Get(),
			Owner);
	}
}
