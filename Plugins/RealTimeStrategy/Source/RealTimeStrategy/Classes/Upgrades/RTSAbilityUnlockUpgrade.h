#pragma once

#include "CoreMinimal.h"

#include "Templates/SubclassOf.h"

#include "Abilities/RTSAbility.h"
#include "Upgrades/RTSUpgrade.h"

#include "RTSAbilityUnlockUpgrade.generated.h"


/** Upgrade that unlocks an ability for all units that have it set as RequiredUpgrade.
 *  Set FRTSAbilityData.RequiredUpgrade to this class on the unit's ability slot. */
UCLASS(BlueprintType, Blueprintable)
class REALTIMESTRATEGY_API URTSAbilityUnlockUpgrade : public URTSUpgrade
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure)
    TSubclassOf<URTSAbility> GetAbilityToUnlock() const { return AbilityToUnlock; }

protected:
    /** The ability class this upgrade unlocks. Used as metadata/UI only;
     *  the actual unlock check is done via URTSPlayerUpgradeComponent::HasUpgrade(). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TSubclassOf<URTSAbility> AbilityToUnlock;
};
