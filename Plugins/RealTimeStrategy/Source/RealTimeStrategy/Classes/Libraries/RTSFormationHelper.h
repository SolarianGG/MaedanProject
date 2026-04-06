#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "RTSFormationHelper.generated.h"


/**
* Utility functions for calculating unit formation positions.
*/
UCLASS()
class REALTIMESTRATEGY_API URTSFormationHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	* Calculates positions for units arranged in concentric circles around a center point.
	* Unit 0 gets the center; remaining units fill rings at increasing radii.
	*
	* @param Center      The target location (e.g. where the player clicked).
	* @param UnitCount   Total number of units to arrange.
	* @param UnitSpacing Distance between units in cm.
	* @return Array of positions, one per unit.
	*/
	UFUNCTION(BlueprintCallable, Category = "RTS")
	static TArray<FVector> CalculateCircularFormation(const FVector& Center, int32 UnitCount, float UnitSpacing = 150.0f);
};
