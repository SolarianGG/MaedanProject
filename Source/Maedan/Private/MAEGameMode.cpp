#include "MAEGameMode.h"

#include "MAEPlayerController.h"

AMAEGameMode::AMAEGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMAEGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);


}
