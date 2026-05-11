#pragma once

#include "CoreMinimal.h"

#include "Templates/SubclassOf.h"

#include "Upgrades/RTSUpgrade.h"

#include "RTSAbilityData.generated.h"


class URTSAbility;


/** Runtime data for an ability slot on a unit. */
USTRUCT(BlueprintType)
struct REALTIMESTRATEGY_API FRTSAbilityData
{
	GENERATED_BODY()

public:
	/** The ability class (defines behavior via CDO). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	TSubclassOf<URTSAbility> AbilityClass;

	/** If set, the player must have researched this upgrade before the ability can be used.
	 *  Leave null to have the ability always available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	TSubclassOf<URTSUpgrade> RequiredUpgrade;

	/** Runtime: remaining cooldown for this ability slot. Not saved; tick-decremented. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS")
	float RemainingCooldown = 0.0f;
};
