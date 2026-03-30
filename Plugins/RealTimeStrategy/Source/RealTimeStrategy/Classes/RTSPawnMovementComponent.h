#pragma once

#include "CoreMinimal.h"

#include "GameFramework/FloatingPawnMovement.h"

#include "RTSPawnMovementComponent.generated.h"


/** Simple pawn movement that also updates rotation and applies separation forces to avoid stacking. */
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSPawnMovementComponent : public UFloatingPawnMovement
{
    GENERATED_BODY()

public:
    URTSPawnMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void UpdateComponentVelocity() override;

private:
    /** Whether to automatically update the pawn rotation to match its velocity. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	bool bUpdateRotation;

    /** Whether to apply separation forces to avoid overlapping with nearby units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Separation")
    bool bEnableSeparation;

    /** Radius to scan for nearby units, in cm. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Separation", meta = (ClampMin = 0, EditCondition = "bEnableSeparation"))
    float SeparationRadius;

    /** Maximum separation force magnitude, in cm/s. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Separation", meta = (ClampMin = 0, EditCondition = "bEnableSeparation"))
    float SeparationStrength;

    /** How often to re-query nearby actors, in seconds. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Separation", meta = (ClampMin = 0.01f, EditCondition = "bEnableSeparation"))
    float SeparationQueryInterval;

    /** Timer accumulator for amortized overlap queries. */
    float SeparationQueryTimer;

    /** Cached list of nearby actors from the last overlap query. */
    UPROPERTY(Transient)
    TArray<AActor*> CachedNearbyActors;

    /** Performs the sphere overlap query and caches results. */
    void UpdateNearbyActorsCache();

    /** Computes the separation velocity from cached nearby actors. */
    FVector ComputeSeparationForce() const;
};
