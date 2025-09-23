// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MAEPlayer.generated.h"

class UMAERTSCameraComponent;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class MAEDAN_API AMAEPlayer : public APawn
{
	GENERATED_BODY()

public:
	AMAEPlayer();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMAERTSCameraComponent> RTSCameraHandleComponent;
	
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
