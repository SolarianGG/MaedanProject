#include "MAEGameMode.h"

#include "MAEPlayerController.h"

AMAEGameMode::AMAEGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMAEGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AccumulatedTime += DeltaSeconds;
	if (AccumulatedTime >= TickInterval)
	{
		CurrentTick++;
		if (Commands.Contains(CurrentTick))
		{
			for (auto& [Command, Actor] : Commands[CurrentTick])
			{
				Command->Execute(Actor);	
			}
			Commands.Remove(CurrentTick);
		}
		AccumulatedTime = 0.f;
	}
}

// TODO: Think about doing it differently
void AMAEGameMode::AddCommand(TPair<TSharedPtr<UMAECommand>, AActor*> Command)
{
	Commands[CurrentTick+1].Add(std::move(Command));
}
