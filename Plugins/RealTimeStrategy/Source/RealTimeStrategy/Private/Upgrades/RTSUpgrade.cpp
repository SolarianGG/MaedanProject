#include "Upgrades/RTSUpgrade.h"

#include "GameFramework/Controller.h"

#include "RTSLog.h"
#include "Upgrades/RTSPlayerUpgradeComponent.h"


void URTSUpgrade::ApplyUpgrade(AController* PlayerController)
{
    if (!IsValid(PlayerController))
    {
        return;
    }

    URTSPlayerUpgradeComponent* UpgradeComp =
        PlayerController->FindComponentByClass<URTSPlayerUpgradeComponent>();

    if (!IsValid(UpgradeComp))
    {
        UE_LOG(LogRTS, Warning,
            TEXT("URTSUpgrade::ApplyUpgrade: %s has no URTSPlayerUpgradeComponent."),
            *PlayerController->GetName());
        return;
    }

    UpgradeComp->AddUpgrade(GetClass());
}

bool URTSUpgrade::CanResearch(AController* PlayerController) const
{
    if (!IsValid(PlayerController))
    {
        return false;
    }

    URTSPlayerUpgradeComponent* UpgradeComp =
        PlayerController->FindComponentByClass<URTSPlayerUpgradeComponent>();

    // Allow research only if not already researched.
    return IsValid(UpgradeComp) && !UpgradeComp->HasUpgrade(GetClass());
}
