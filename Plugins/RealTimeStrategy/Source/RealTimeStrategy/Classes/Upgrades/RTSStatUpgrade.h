#pragma once

#include "CoreMinimal.h"

#include "Upgrades/RTSUpgrade.h"

#include "RTSStatUpgrade.generated.h"


/** Upgrade that adds a flat bonus to MaximumHealth and/or MaximumMana
 *  of all owned units. Applied immediately to existing units and automatically
 *  to units spawned afterwards via URTSPlayerUpgradeComponent::ApplyStatUpgradesToActor(). */
UCLASS(BlueprintType, Blueprintable)
class REALTIMESTRATEGY_API URTSStatUpgrade : public URTSUpgrade
{
    GENERATED_BODY()

public:
    virtual void ApplyUpgrade(AController* PlayerController) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float MaxHealthBonus;

    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float MaxManaBonus;
};
