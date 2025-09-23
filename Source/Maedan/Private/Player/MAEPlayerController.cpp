// Project Maedan, all right incorporated.

#include "Player/MAEPlayerController.h"

void AMAEPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameAndUI());
	SetShowMouseCursor(true);
}
