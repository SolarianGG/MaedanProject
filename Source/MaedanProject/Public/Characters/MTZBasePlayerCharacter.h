// Maedan Project, All rights incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Characters/MTZBaseCharacter.h"
#include "MTZBasePlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
struct FInputActionValue;
class UInputAction;
/**
 * 
 */
UCLASS()
class MAEDANPROJECT_API AMTZBasePlayerCharacter : public AMTZBaseCharacter
{
	GENERATED_BODY()

public:
	AMTZBasePlayerCharacter();
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent{nullptr};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> MoveInputAction{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction>  LookInputAction{nullptr};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> JumpInputAction{nullptr};
private:
	void OnMove(const FInputActionValue& InputValue);
	void OnLook(const FInputActionValue& InputValue);
};
