#include "Upgrades/RTSStatUpgrade.h"

#include "GameFramework/Controller.h"

#include "RTSLog.h"
#include "Upgrades/RTSPlayerUpgradeComponent.h"


void URTSStatUpgrade::ApplyUpgrade(AController* PlayerController)
{
    if (!IsValid(PlayerController))
    {
        return;
    }

    URTSPlayerUpgradeComponent* UpgradeComp =
        PlayerController->FindComponentByClass<URTSPlayerUpgradeComponent>();

    if (!IsValid(UpgradeComp) || UpgradeComp->HasUpgrade(GetClass()))
    {
        return;
    }

    // Record upgrade and fire OnUpgradeResearched.
    Super::ApplyUpgrade(PlayerController);

    if (MaxHealthBonus <= 0.f && MaxManaBonus <= 0.f)
    {
        return;
    }

    UpgradeComp->AddStatBonus(MaxHealthBonus, MaxManaBonus);

    UE_LOG(LogRTS, Log, TEXT("URTSStatUpgrade %s applied for %s: +%.1f HP, +%.1f Mana."),
        *GetName(), *PlayerController->GetName(), MaxHealthBonus, MaxManaBonus);
}
