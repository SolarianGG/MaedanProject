// Project Maedan, all right incorporated.


#include "Player/MAEPlayer.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/Components/MAERTSCameraComponent.h"

AMAEPlayer::AMAEPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("Spring Arm Component");
	SpringArmComponent->SetupAttachment(GetRootComponent());
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("Camera Component");
	CameraComponent->SetupAttachment(SpringArmComponent);

	CameraComponent->bUsePawnControlRotation = false;

	RTSCameraHandleComponent = CreateDefaultSubobject<UMAERTSCameraComponent>("RTSCameraHandleComponent");
}

void AMAEPlayer::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMAEPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMAEPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

