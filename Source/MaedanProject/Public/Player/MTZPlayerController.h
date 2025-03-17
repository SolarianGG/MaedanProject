// Maedan Project, All rights incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MTZPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class MAEDANPROJECT_API AMTZPlayerController : public APlayerController
{
	GENERATED_BODY()
protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TSoftObjectPtr<class UInputMappingContext> InputMappingContext{nullptr};
	
	virtual void BeginPlay() override;

	
};
