#include "AI/RTSBTDecorator_HasAttackMoveOrder.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Orders/RTSAttackMoveOrder.h"

URTSBTDecorator_HasAttackMoveOrder::URTSBTDecorator_HasAttackMoveOrder(
    const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    NodeName = TEXT("Has Attack Move Order");
}

bool URTSBTDecorator_HasAttackMoveOrder::CalculateRawConditionValue(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        return false;
    }

    const UClass* OrderClass = Cast<UClass>(BB->GetValueAsClass(FName(TEXT("OrderClass"))));
    return OrderClass == URTSAttackMoveOrder::StaticClass();
}
