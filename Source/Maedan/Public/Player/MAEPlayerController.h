// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "GameFramework/PlayerController.h"
#include "MAEPlayerController.generated.h"

class UInputAction;
class UMAESelectionComponent;
class UInputMappingContext;
class AMAEGameMode;

/**
 * 
 */
UCLASS()
class MAEDAN_API AMAEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMAEPlayerController();

protected:
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

	virtual void SetupInputComponent() override;


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Select")
	TObjectPtr<UMAESelectionComponent> SelectionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputMappingContext> CameraInputMappings;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> UnitInputMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> SelectAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> MultipleSelection;

private:
	FVector2D SelectionStartPosition;
	bool bIsSelecting = false;

	void OnSelectionStart();
	void OnSelectionUpdate();
	void OnSelectionEnd();
	void OnSelectActor();
};
