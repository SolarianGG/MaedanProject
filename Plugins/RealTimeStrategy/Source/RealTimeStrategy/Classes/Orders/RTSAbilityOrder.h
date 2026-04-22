#pragma once

#include "CoreMinimal.h"

#include "Orders/RTSOrder.h"

#include "RTSAbilityOrder.generated.h"


/** Orders the actor to cast an ability. The Index field in FRTSOrderData maps to the ability index. */
UCLASS()
class REALTIMESTRATEGY_API URTSAbilityOrder : public URTSOrder
{
    GENERATED_BODY()

public:
    URTSAbilityOrder(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual bool CanObeyOrder(const AActor* OrderedActor, int32 Index) const override;
    virtual bool IsValidTarget(const AActor* OrderedActor, const FRTSOrderTargetData& TargetData, int32 Index) const override;
    virtual void IssueOrder(AActor* OrderedActor, const FRTSOrderTargetData& TargetData, int32 Index) const override;
};
