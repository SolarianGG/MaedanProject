// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MAEPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MAEDAN_API AMAEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

protected:
	virtual void BeginPlay() override;

private:
};
