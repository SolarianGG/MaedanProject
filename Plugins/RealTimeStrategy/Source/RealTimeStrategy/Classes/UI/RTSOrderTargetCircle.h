#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSOrderTargetCircle.generated.h"

class UDecalComponent;
class UMaterialInterface;


/**
 * Temporary decal actor spawned on an enemy or resource when an order is issued.
 * Fades out and destroys itself after Duration seconds.
 */
UCLASS()
class REALTIMESTRATEGY_API ARTSOrderTargetCircle : public AActor
{
    GENERATED_BODY()

public:
    ARTSOrderTargetCircle();
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    UDecalComponent* DecalComponent;

    /** Material for the circle. Set in Blueprint subclass. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UMaterialInterface* CircleMaterial;

    /** How long the circle is visible before fading out (seconds). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    float Duration;
};
