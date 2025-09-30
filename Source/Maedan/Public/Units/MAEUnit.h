// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MAESelectableInterface.h"
#include "MAEUnit.generated.h"

UCLASS()
class MAEDAN_API AMAEUnit : public ACharacter, public IMAESelectableInterface
{
	GENERATED_BODY()

public:
	AMAEUnit();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual bool IsSelectable_Implementation() const override;
	virtual void OnDeselected_Implementation() override;
	virtual void OnSelected_Implementation() override;
};
