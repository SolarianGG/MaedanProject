#pragma once

#include "CoreMinimal.h"

#include "Templates/SubclassOf.h"

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

	/** Runtime: remaining cooldown for this ability slot. Not saved; tick-decremented. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS")
	float RemainingCooldown = 0.0f;
};
