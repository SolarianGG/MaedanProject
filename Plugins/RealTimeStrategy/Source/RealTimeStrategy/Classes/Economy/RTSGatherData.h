#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"

#include "Templates/SubclassOf.h"

#include "Economy/RTSResourceType.h"

#include "RTSGatherData.generated.h"


USTRUCT(BlueprintType)
struct REALTIMESTRATEGY_API FRTSGatherData
{
	GENERATED_BODY()

public:
	/** Type of resources that can be gathered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	TSubclassOf<URTSResourceType> ResourceType;

	/** Amount of resources gathered after each cooldown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float AmountPerGathering;

	/** Maximum amount of resources that can be carried. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float Capacity;

	/** Time between two gatherings, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float Cooldown;

	/** Anim montage to play when unit is collecting resources */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	UAnimMontage* CollectingAnimMontage = nullptr;

	/** Whether the actor needs to return to the drain in order to drop resources. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	bool bNeedsReturnToDrain;

	/** Range in which resources can be gathered, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float Range;
};
