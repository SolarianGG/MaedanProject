#include "Libraries/RTSFormationHelper.h"


TArray<FVector> URTSFormationHelper::CalculateCircularFormation(const FVector& Center, int32 UnitCount, float UnitSpacing)
{
	TArray<FVector> Positions;

	if (UnitCount <= 0)
	{
		return Positions;
	}

	Positions.Reserve(UnitCount);

	// First unit gets the exact center.
	Positions.Add(Center);

	if (UnitCount == 1)
	{
		return Positions;
	}

	// Fill concentric rings.
	int32 Placed = 1;
	int32 Ring = 1;

	while (Placed < UnitCount)
	{
		const float Radius = Ring * UnitSpacing;
		const float Circumference = 2.0f * PI * Radius;
		const int32 MaxInRing = FMath::Max(1, FMath::FloorToInt(Circumference / UnitSpacing));
		const int32 UnitsInRing = FMath::Min(MaxInRing, UnitCount - Placed);
		const float AngleStep = 2.0f * PI / UnitsInRing;

		for (int32 i = 0; i < UnitsInRing; ++i)
		{
			const float Angle = i * AngleStep;
			FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
			Positions.Add(Center + Offset);
			++Placed;
		}

		++Ring;
	}

	return Positions;
}
