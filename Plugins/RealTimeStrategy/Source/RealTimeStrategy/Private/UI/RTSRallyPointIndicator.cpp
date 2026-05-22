#include "UI/RTSRallyPointIndicator.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"


ARTSRallyPointIndicator::ARTSRallyPointIndicator()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    TravelNiagaraComponent = nullptr;
    ArrivalNiagaraComponent = nullptr;
    TravelEffectSystem = nullptr;
    ArrivalEffectSystem = nullptr;
    TotalDistance = 0.0f;
    Alpha = 0.0f;
    bInitialized = false;
    bEffectsActive = false;
}

void ARTSRallyPointIndicator::Initialize(UNiagaraSystem* TravelEffect, UNiagaraSystem* ArrivalEffect,
                                          const FVector& Start, const FVector& End)
{
    StartLocation = Start + FVector(0.0f, 0.0f, HeightOffset);
    EndLocation = End + FVector(0.0f, 0.0f, HeightOffset);
    TravelEffectSystem = TravelEffect;
    ArrivalEffectSystem = ArrivalEffect;
    TotalDistance = FVector::Dist(StartLocation, EndLocation);

    if (TotalDistance < 1.0f)
    {
        Destroy();
        return;
    }

    bInitialized = true;
}

void ARTSRallyPointIndicator::StartTravelLoop()
{
    Alpha = 0.0f;
    SetActorLocation(StartLocation);

    FRotator LookAtBuilding = (StartLocation - EndLocation).Rotation();
    SetActorRotation(LookAtBuilding);

    if (TravelNiagaraComponent)
    {
        TravelNiagaraComponent->DeactivateImmediate();
        TravelNiagaraComponent->DestroyComponent();
        TravelNiagaraComponent = nullptr;
    }

    if (TravelEffectSystem)
    {
        TravelNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            TravelEffectSystem,
            GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            false);
    }

    bEffectsActive = true;
}

void ARTSRallyPointIndicator::StopAllEffects()
{
    if (TravelNiagaraComponent)
    {
        TravelNiagaraComponent->DeactivateImmediate();
        TravelNiagaraComponent->DestroyComponent();
        TravelNiagaraComponent = nullptr;
    }

    if (ArrivalNiagaraComponent)
    {
        ArrivalNiagaraComponent->DeactivateImmediate();
        ArrivalNiagaraComponent->DestroyComponent();
        ArrivalNiagaraComponent = nullptr;
    }

    bEffectsActive = false;
}

void ARTSRallyPointIndicator::Destroyed()
{
    StopAllEffects();
    Super::Destroyed();
}

void ARTSRallyPointIndicator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bInitialized)
    {
        return;
    }

    if (!bEffectsActive)
    {
        StartTravelLoop();
        return;
    }

    Alpha += DeltaTime / TravelDuration;

    if (Alpha >= 1.0f)
    {
        if (ArrivalNiagaraComponent)
        {
            ArrivalNiagaraComponent->DeactivateImmediate();
            ArrivalNiagaraComponent->DestroyComponent();
            ArrivalNiagaraComponent = nullptr;
        }

        if (ArrivalEffectSystem)
        {
            ArrivalNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), ArrivalEffectSystem, EndLocation, FRotator::ZeroRotator,
                FVector(1.0f), false);
        }

        StartTravelLoop();
        return;
    }

    FVector CurrentLocation = FMath::Lerp(StartLocation, EndLocation, Alpha);
    SetActorLocation(CurrentLocation);

    FVector DirectionToBuilding = (StartLocation - CurrentLocation).GetSafeNormal();
    if (!DirectionToBuilding.IsNearlyZero())
    {
        SetActorRotation(DirectionToBuilding.Rotation());
    }
}
