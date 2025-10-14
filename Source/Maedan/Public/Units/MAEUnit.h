// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MAESelectableInterface.h"
#include "MAEUnit.generated.h"

class UInputAction;
UCLASS()
class MAEDAN_API AMAEUnit : public ACharacter, public IMAESelectableInterface
{
	GENERATED_BODY()

public:
	AMAEUnit();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> UnitInputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveInputAction;

	virtual UInputMappingContext* GetInputMapping_Implementation() const override { return UnitInputMapping; }

	virtual void SetupSelectionInput_Implementation(UEnhancedInputComponent* InputComp, APlayerController* PC) override;

	virtual void OnSelected_Implementation(APlayerController* PC) override;

	virtual void OnDeselected_Implementation(APlayerController* PC) override;

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void Move();

private:
	UPROPERTY();
	APlayerController* OwningPlayer;

};
