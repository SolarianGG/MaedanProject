#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "RTSBTDecorator_HasAttackMoveOrder.generated.h"

/** Behavior tree decorator that checks whether the pawn's current order is an attack-move order. */
UCLASS()
class REALTIMESTRATEGY_API URTSBTDecorator_HasAttackMoveOrder : public UBTDecorator
{
    GENERATED_BODY()

public:
    URTSBTDecorator_HasAttackMoveOrder(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
