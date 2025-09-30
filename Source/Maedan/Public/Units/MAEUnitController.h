// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MAEUnitController.generated.h"

/**
 * 
 */
UCLASS()
class MAEDAN_API AMAEUnitController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
