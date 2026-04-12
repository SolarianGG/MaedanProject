#include "Orders/RTSAbilityOrder.h"

#include "GameFramework/Actor.h"

#include "Abilities/RTSAbility.h"
#include "Abilities/RTSAbilitySystemComponent.h"
#include "Libraries/RTSGameplayTagLibrary.h"


URTSAbilityOrder::URTSAbilityOrder(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    TargetType = ERTSOrderTargetType::ORDERTARGET_Actor;
    GroupExecutionType = ERTSOrderGroupExecutionType::ORDERGROUPEXECUTION_All;

    IssueTagRequirements.SourceRequiredTags.AddTag(URTSGameplayTagLibrary::Status_Permanent_HasAbilities());
    IssueTagRequirements.SourceRequiredTags.AddTag(URTSGameplayTagLibrary::Status_Changing_Alive());
}

bool URTSAbilityOrder::CanObeyOrder(const AActor* OrderedActor, int32 Index) const
{
    if (!IsValid(OrderedActor))
    {
        return false;
    }

    URTSAbilitySystemComponent* AbilitySystem = OrderedActor->FindComponentByClass<URTSAbilitySystemComponent>();

    if (!IsValid(AbilitySystem))
    {
        return false;
    }

    TArray<FRTSAbilityData> Abilities = AbilitySystem->GetAbilities();

    if (!Abilities.IsValidIndex(Index))
    {
        return false;
    }

    if (!Abilities[Index].AbilityClass)
    {
        return false;
    }

    const URTSAbility* AbilityCDO = Abilities[Index].AbilityClass->GetDefaultObject<URTSAbility>();
    return AbilityCDO->CanActivateAbility(OrderedActor, Index);
}

bool URTSAbilityOrder::IsValidTarget(const AActor* OrderedActor, const FRTSOrderTargetData& TargetData, int32 Index) const
{
    if (!IsValid(OrderedActor))
    {
        return false;
    }

    URTSAbilitySystemComponent* AbilitySystem = OrderedActor->FindComponentByClass<URTSAbilitySystemComponent>();

    if (!IsValid(AbilitySystem))
    {
        return false;
    }

    TArray<FRTSAbilityData> Abilities = AbilitySystem->GetAbilities();

    if (!Abilities.IsValidIndex(Index))
    {
        return false;
    }

    if (!Abilities[Index].AbilityClass)
    {
        return false;
    }

    const URTSAbility* AbilityCDO = Abilities[Index].AbilityClass->GetDefaultObject<URTSAbility>();
    return AbilityCDO->IsValidAbilityTarget(OrderedActor, TargetData);
}

void URTSAbilityOrder::IssueOrder(AActor* OrderedActor, const FRTSOrderTargetData& TargetData, int32 Index) const
{
    if (!IsValid(OrderedActor))
    {
        return;
    }

    URTSAbilitySystemComponent* AbilitySystem = OrderedActor->FindComponentByClass<URTSAbilitySystemComponent>();

    if (!IsValid(AbilitySystem))
    {
        return;
    }

    AbilitySystem->UseAbility(Index, TargetData.Actor, TargetData.Location);
}
