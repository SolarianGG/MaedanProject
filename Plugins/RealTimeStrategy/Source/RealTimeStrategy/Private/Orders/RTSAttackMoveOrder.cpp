#include "Orders/RTSAttackMoveOrder.h"

#include "Libraries/RTSGameplayTagLibrary.h"
#include "Orders/RTSOrderTargetType.h"

URTSAttackMoveOrder::URTSAttackMoveOrder(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    TargetType = ERTSOrderTargetType::ORDERTARGET_Location;
    GroupExecutionType = ERTSOrderGroupExecutionType::ORDERGROUPEXECUTION_All;

    IssueTagRequirements.SourceBlockedTags.AddTag(URTSGameplayTagLibrary::Status_Changing_Immobilized());
    IssueTagRequirements.SourceBlockedTags.AddTag(URTSGameplayTagLibrary::Status_Changing_Constructing());
}
