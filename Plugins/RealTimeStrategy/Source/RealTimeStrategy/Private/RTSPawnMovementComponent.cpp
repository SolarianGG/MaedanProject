#include "RTSPawnMovementComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Pawn.h"

#include "RTSOwnerComponent.h"


URTSPawnMovementComponent::URTSPawnMovementComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    // Set reasonable default values.
    bUpdateRotation = true;
    bEnableSeparation = true;
    SeparationRadius = 200.0f;
    SeparationStrength = 300.0f;
    SeparationQueryInterval = 0.1f;
    SeparationQueryTimer = 0.0f;
}

void URTSPawnMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEnableSeparation || !UpdatedComponent)
    {
        return;
    }

    // Amortized overlap query.
    SeparationQueryTimer += DeltaTime;
    if (SeparationQueryTimer >= SeparationQueryInterval)
    {
        SeparationQueryTimer = 0.0f;
        UpdateNearbyActorsCache();
    }

    // Compute and apply separation force every tick.
    FVector SepForce = ComputeSeparationForce();
    if (!SepForce.IsNearlyZero())
    {
        AddInputVector(SepForce);
    }
}

void URTSPawnMovementComponent::UpdateComponentVelocity()
{
    Super::UpdateComponentVelocity();

    if (bUpdateRotation && !Velocity.IsNearlyZero())
    {
        MoveUpdatedComponent(FVector::ZeroVector, Velocity.Rotation(), false);
    }
}

void URTSPawnMovementComponent::UpdateNearbyActorsCache()
{
    CachedNearbyActors.Reset();

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(Owner);

    UKismetSystemLibrary::SphereOverlapActors(
        this,
        Owner->GetActorLocation(),
        SeparationRadius,
        TArray<TEnumAsByte<EObjectTypeQuery>>(), // All object types.
        APawn::StaticClass(),
        ActorsToIgnore,
        CachedNearbyActors);
}

FVector URTSPawnMovementComponent::ComputeSeparationForce() const
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || CachedNearbyActors.Num() == 0)
    {
        return FVector::ZeroVector;
    }

    const URTSOwnerComponent* MyOwnerComponent = Owner->FindComponentByClass<URTSOwnerComponent>();
    const FVector MyLocation = Owner->GetActorLocation();

    FVector TotalForce = FVector::ZeroVector;

    for (AActor* Other : CachedNearbyActors)
    {
        if (!IsValid(Other))
        {
            continue;
        }

        // Only separate from same-team units.
        if (MyOwnerComponent && !MyOwnerComponent->IsSameTeamAsActor(Other))
        {
            continue;
        }

        const FVector OtherLocation = Other->GetActorLocation();
        FVector Delta = MyLocation - OtherLocation;
        Delta.Z = 0.0f; // 2D only to prevent flying.

        const float Distance = Delta.Size();

        if (Distance > SeparationRadius)
        {
            continue;
        }

        if (Distance > SMALL_NUMBER)
        {
            // Linear falloff: stronger when closer.
            const float Weight = 1.0f - (Distance / SeparationRadius);
            TotalForce += Delta.GetSafeNormal() * Weight;
        }
        else
        {
            // Perfect overlap: random offset to break symmetry.
            TotalForce += FVector(
                FMath::FRandRange(-1.0f, 1.0f),
                FMath::FRandRange(-1.0f, 1.0f),
                0.0f).GetSafeNormal();
        }
    }

    // Clamp and scale.
    if (!TotalForce.IsNearlyZero())
    {
        TotalForce = TotalForce.GetClampedToMaxSize(1.0f) * SeparationStrength;
    }

    return TotalForce;
}
