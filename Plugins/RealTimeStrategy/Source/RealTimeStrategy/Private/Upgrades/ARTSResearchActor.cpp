#include "Upgrades/ARTSResearchActor.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

#include "RTSDescriptionComponent.h"
#include "RTSLog.h"
#include "RTSNameComponent.h"
#include "RTSPortraitComponent.h"
#include "Production/RTSProductionCostComponent.h"
#include "RTSRequirementsComponent.h"
#include "Upgrades/RTSUpgrade.h"


ARTSResearchActor::ARTSResearchActor(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    ProductionCostComponent = CreateDefaultSubobject<URTSProductionCostComponent>(TEXT("ProductionCostComponent"));
    RequirementsComponent   = CreateDefaultSubobject<URTSRequirementsComponent>(TEXT("RequirementsComponent"));
    NameComponent           = CreateDefaultSubobject<URTSNameComponent>(TEXT("NameComponent"));
    PortraitComponent       = CreateDefaultSubobject<URTSPortraitComponent>(TEXT("PortraitComponent"));
    DescriptionComponent    = CreateDefaultSubobject<URTSDescriptionComponent>(TEXT("DescriptionComponent"));
}

void ARTSResearchActor::PostLoad()
{
    Super::PostLoad();
    SyncDisplayComponents();
}

#if WITH_EDITOR
void ARTSResearchActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    SyncDisplayComponents();
}
#endif

bool ARTSResearchActor::CanResearchUpgrade(AController* PlayerController) const
{
    if (!UpgradeClass)
    {
        return false;
    }

    const URTSUpgrade* UpgradeCDO = UpgradeClass->GetDefaultObject<URTSUpgrade>();
    return UpgradeCDO->CanResearch(PlayerController);
}

void ARTSResearchActor::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    // Defer one tick so that ARTSGameMode::TransferOwnership() (called right after
    // SpawnActorForPlayer returns) has a chance to set GetOwner() to the player controller.
    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ARTSResearchActor::ApplyUpgradeAndDestroy);
}

void ARTSResearchActor::SyncDisplayComponents()
{
    if (!UpgradeClass)
    {
        return;
    }

    const URTSUpgrade* UpgradeCDO = UpgradeClass->GetDefaultObject<URTSUpgrade>();

    NameComponent->SetName(UpgradeCDO->GetUpgradeName());
    PortraitComponent->SetPortrait(UpgradeCDO->GetUpgradeIcon());
    DescriptionComponent->SetDescription(UpgradeCDO->GetUpgradeDescription());
}

void ARTSResearchActor::ApplyUpgradeAndDestroy()
{
    if (!UpgradeClass)
    {
        UE_LOG(LogRTS, Warning, TEXT("ARTSResearchActor %s: UpgradeClass is not set."), *GetName());
        Destroy();
        return;
    }

    AController* PC = Cast<AController>(GetOwner());

    if (!IsValid(PC))
    {
        UE_LOG(LogRTS, Warning,
            TEXT("ARTSResearchActor %s: could not find owning controller. Upgrade not applied."),
            *GetName());
        Destroy();
        return;
    }

    URTSUpgrade* UpgradeCDO = UpgradeClass->GetDefaultObject<URTSUpgrade>();
    UpgradeCDO->ApplyUpgrade(PC);

    UE_LOG(LogRTS, Log, TEXT("ARTSResearchActor %s: applied upgrade %s for player %s."),
        *GetName(), *UpgradeClass->GetName(), *PC->GetName());

    Destroy();
}
