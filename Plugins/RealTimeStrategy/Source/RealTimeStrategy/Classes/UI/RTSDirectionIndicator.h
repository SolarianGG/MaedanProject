#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSDirectionIndicator.generated.h"

class UDecalComponent;

/** Draws a line from caster to target location during ability targeting. */
UCLASS()
class REALTIMESTRATEGY_API ARTSDirectionIndicator : public AActor
{
    GENERATED_BODY()

public:
    ARTSDirectionIndicator();

    virtual void BeginPlay() override;

    /** Updates the line endpoints. Called every tick during ability targeting. */
    UFUNCTION(BlueprintCallable, Category = RTS)
    void SetTarget(FVector InStart, FVector InEnd);

    /** Material to apply to the decal. Must be a Deferred Decal material. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UMaterialInterface* LineMaterial;

    /** Half-width of the line in world units (cm). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
    float LineHalfWidth = 12.5f;

    /** How far the decal projects downward (cm). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 1))
    float ProjectionDepth = 200.0f;

private:
    UPROPERTY()
    UDecalComponent* DecalComponent;
};
