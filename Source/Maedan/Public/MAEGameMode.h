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

protected:


private:
};
