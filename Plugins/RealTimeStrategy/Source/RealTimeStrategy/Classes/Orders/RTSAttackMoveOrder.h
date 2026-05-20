#pragma once

#include "CoreMinimal.h"
#include "Orders/RTSOrder.h"
#include "RTSAttackMoveOrder.generated.h"

/** Orders the actor to move to the specified location and attack any enemies encountered on the way. */
UCLASS()
class REALTIMESTRATEGY_API URTSAttackMoveOrder : public URTSOrder
{
    GENERATED_BODY()

public:
    URTSAttackMoveOrder(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
