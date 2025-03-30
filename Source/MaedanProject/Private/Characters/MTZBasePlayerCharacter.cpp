// Maedan Project, All rights incorporated.


#include "Characters/MTZBasePlayerCharacter.h"

#include "MTZPlayerState.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AMTZBasePlayerCharacter::AMTZBasePlayerCharacter()
	: Super()
{
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("Spring Arm Component");
	SpringArmComponent->SetupAttachment(GetRootComponent());

	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera Component");
	CameraComponent->SetupAttachment(SpringArmComponent);

	SpringArmComponent->bUsePawnControlRotation = true;
}

void AMTZBasePlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (auto* PS{ GetPlayerState<AMTZPlayerState>() })
	{
		AbilitySystemComponent = Cast<UMTZAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}


void AMTZBasePlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (auto* PS{ GetPlayerState<AMTZPlayerState>() })
	{
		AbilitySystemComponent = Cast<UMTZAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}

void AMTZBasePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	check(LookInputAction);
	check(JumpInputAction);
	check(MoveInputAction);
}

void AMTZBasePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (auto* EInputComponent{ Cast<UEnhancedInputComponent>(PlayerInputComponent) })
	{
		EInputComponent->BindAction(JumpInputAction, ETriggerEvent::Started, this, &ThisClass::Jump);
		EInputComponent->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);

		EInputComponent->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &ThisClass::OnMove);
		EInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &ThisClass::OnLook);
	}
}

void AMTZBasePlayerCharacter::OnMove(const FInputActionValue& InputValue)
{
	const FVector2D LocoVectorValue{ InputValue.Get<FVector2D>() };

	AddMovementInput(GetActorForwardVector(), LocoVectorValue.Y);
	AddMovementInput(GetActorRightVector(), LocoVectorValue.X);	
}

void AMTZBasePlayerCharacter::OnLook(const FInputActionValue& InputValue)
{
	const FVector2D MouseMovementVector{ InputValue.Get<FVector2D>() };

	AddControllerYawInput(MouseMovementVector.X);
	AddControllerPitchInput(-MouseMovementVector.Y);
}