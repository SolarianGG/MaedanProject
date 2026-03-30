#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSRallyPointIndicator.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class URTSProductionComponent;


/**
 * Persistent actor that loops a travel Niagara effect from a building
 * to its rally point, spawning an arrival effect each time it arrives.
 * Only active while the building is producing. Destroyed externally
 * when the rally point changes or is cleared.
 */
UCLASS()
class REALTIMESTRATEGY_API ARTSRallyPointIndicator : public AActor
{
    GENERATED_BODY()

public:
    ARTSRallyPointIndicator();
    virtual void Tick(float DeltaTime) override;
    virtual void Destroyed() override;

    void Initialize(UNiagaraSystem* TravelEffect, UNiagaraSystem* ArrivalEffect,
                    const FVector& Start, const FVector& End, AActor* Building);

private:
    void StartTravelLoop();
    void StopAllEffects();

    UPROPERTY()
    UNiagaraComponent* TravelNiagaraComponent;

    UPROPERTY()
    UNiagaraComponent* ArrivalNiagaraComponent;

    UPROPERTY()
    UNiagaraSystem* TravelEffectSystem;

    UPROPERTY()
    UNiagaraSystem* ArrivalEffectSystem;

    UPROPERTY()
    URTSProductionComponent* ProductionComponent;

    FVector StartLocation;
    FVector EndLocation;
    float TotalDistance;
    float Alpha;
    bool bInitialized;
    bool bEffectsActive;

    static constexpr float HeightOffset = 100.0f;
    static constexpr float TravelDuration = 3.0f;
};
