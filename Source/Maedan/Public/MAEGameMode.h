#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MAEGameMode.generated.h"

class UMAECommand;

UCLASS()
class MAEDAN_API AMAEGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AMAEGameMode();
	virtual void Tick(float DeltaSeconds) override;
	// TODO: Think about doing it differently
	void AddCommand(TPair<TSharedPtr<UMAECommand>, AActor*> Command);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Commands")
	float TickInterval = 1000.0f / 60.0f;
	

private:
	uint64 CurrentTick = 0;
	float AccumulatedTime = 0;

	// TODO: Think about doing it differently
	TMap<uint64, TArray<TPair<TSharedPtr<UMAECommand>, AActor*>>> Commands;
};
