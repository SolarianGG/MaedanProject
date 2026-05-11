#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "Templates/SubclassOf.h"

#include "ARTSResearchActor.generated.h"


class URTSDescriptionComponent;
class URTSNameComponent;
class URTSPortraitComponent;
class URTSProductionCostComponent;
class URTSRequirementsComponent;
class URTSUpgrade;


/** Minimal actor used as a "product" in URTSProductionComponent to represent research.
 *  Has no mesh. On BeginPlay (deferred one tick) it applies UpgradeClass to the owning
 *  player and immediately destroys itself.
 *
 *  Name, portrait, and description are read by the production UI from URTSNameComponent /
 *  URTSPortraitComponent / URTSDescriptionComponent. These are synced from UpgradeClass
 *  automatically in PostLoad (runtime) and PostEditChangeProperty (editor). */
UCLASS(BlueprintType, Blueprintable)
class REALTIMESTRATEGY_API ARTSResearchActor : public AActor
{
    GENERATED_BODY()

public:
    ARTSResearchActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /** Returns false if the upgrade has already been researched or UpgradeClass is not set.
     *  Called by URTSProductionComponent before starting production to prevent re-research. */
    bool CanResearchUpgrade(AController* PlayerController) const;

protected:
    virtual void BeginPlay() override;

    /** Upgrade to apply when this actor is spawned (i.e., research finishes). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TSubclassOf<URTSUpgrade> UpgradeClass;

    /** Required by URTSProductionComponent: defines research time and resource cost. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    URTSProductionCostComponent* ProductionCostComponent;

    /** Required by URTSProductionComponent: defines prerequisite buildings/units. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    URTSRequirementsComponent* RequirementsComponent;

    /** Provides display name to the production UI — synced from UpgradeClass automatically. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    URTSNameComponent* NameComponent;

    /** Provides portrait/icon to the production UI — synced from UpgradeClass automatically. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    URTSPortraitComponent* PortraitComponent;

    /** Provides description to the production UI — synced from UpgradeClass automatically. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    URTSDescriptionComponent* DescriptionComponent;

private:
    /** Copies name/portrait/description from UpgradeClass CDO into the display components. */
    void SyncDisplayComponents();

    UFUNCTION()
    void ApplyUpgradeAndDestroy();
};
