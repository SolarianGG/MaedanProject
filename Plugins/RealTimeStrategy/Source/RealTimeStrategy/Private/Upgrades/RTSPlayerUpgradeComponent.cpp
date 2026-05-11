#include "Upgrades/RTSPlayerUpgradeComponent.h"

#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

#include "RTSLog.h"
#include "RTSPlayerState.h"
#include "Combat/RTSHealthComponent.h"
#include "Combat/RTSManaComponent.h"


URTSPlayerUpgradeComponent::URTSPlayerUpgradeComponent()
{
    SetIsReplicatedByDefault(true);
}

void URTSPlayerUpgradeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URTSPlayerUpgradeComponent, ResearchedUpgrades);
    DOREPLIFETIME(URTSPlayerUpgradeComponent, TotalMaxHealthBonus);
    DOREPLIFETIME(URTSPlayerUpgradeComponent, TotalMaxManaBonus);
}

bool URTSPlayerUpgradeComponent::HasUpgrade(TSubclassOf<URTSUpgrade> UpgradeClass) const
{
    if (!UpgradeClass)
    {
        return false;
    }

    return ResearchedUpgrades.Contains(UpgradeClass);
}

void URTSPlayerUpgradeComponent::AddUpgrade(TSubclassOf<URTSUpgrade> UpgradeClass)
{
    if (!UpgradeClass || ResearchedUpgrades.Contains(UpgradeClass))
    {
        return;
    }

    ResearchedUpgrades.Add(UpgradeClass);

    UE_LOG(LogRTS, Log, TEXT("URTSPlayerUpgradeComponent: %s researched upgrade %s."),
        *GetOwner()->GetName(), *UpgradeClass->GetName());

    OnUpgradeResearched.Broadcast(UpgradeClass);
}

void URTSPlayerUpgradeComponent::AddStatBonus(float HealthBonus, float ManaBonus)
{
    TotalMaxHealthBonus += HealthBonus;
    TotalMaxManaBonus   += ManaBonus;

    // Apply updated totals to all currently owned units.
    AController* PC = Cast<AController>(GetOwner());

    if (!IsValid(PC))
    {
        return;
    }

    ARTSPlayerState* PS = PC->GetPlayerState<ARTSPlayerState>();

    if (!IsValid(PS))
    {
        return;
    }

    for (AActor* Actor : PS->GetOwnActors())
    {
        ApplyStatUpgradesToActor(Actor);
    }
}

void URTSPlayerUpgradeComponent::ApplyStatUpgradesToActor(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    if (TotalMaxHealthBonus > 0.f)
    {
        URTSHealthComponent* HC = Actor->FindComponentByClass<URTSHealthComponent>();

        if (IsValid(HC))
        {
            HC->SetMaximumHealthBonus(TotalMaxHealthBonus);
        }
    }

    if (TotalMaxManaBonus > 0.f)
    {
        URTSManaComponent* MC = Actor->FindComponentByClass<URTSManaComponent>();

        if (IsValid(MC))
        {
            MC->SetMaximumManaBonus(TotalMaxManaBonus);
        }
    }
}

void URTSPlayerUpgradeComponent::OnRep_ResearchedUpgrades()
{
    if (ResearchedUpgrades.Num() > 0)
    {
        // Broadcast the most recently added upgrade.
        OnUpgradeResearched.Broadcast(ResearchedUpgrades.Last());
    }
}
