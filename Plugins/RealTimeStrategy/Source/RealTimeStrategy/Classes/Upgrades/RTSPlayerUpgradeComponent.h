#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "Templates/SubclassOf.h"

#include "Upgrades/RTSUpgrade.h"

#include "RTSPlayerUpgradeComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSPlayerUpgradeComponentUpgradeResearchedSignature,
    TSubclassOf<URTSUpgrade>, UpgradeClass);


/** Attached to PlayerController. Tracks all upgrades researched by this player.
 *  Replicated so clients can display correct UI (locked abilities, stat bars). */
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSPlayerUpgradeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSPlayerUpgradeComponent();

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Whether the specified upgrade has been researched. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    bool HasUpgrade(TSubclassOf<URTSUpgrade> UpgradeClass) const;

    /** Records an upgrade as researched and fires OnUpgradeResearched. Server-only.
     *  Called automatically by URTSUpgrade::ApplyUpgrade(). */
    void AddUpgrade(TSubclassOf<URTSUpgrade> UpgradeClass);

    /** Adds stat bonuses to the running totals and applies them to all currently
     *  owned units. Server-only. Called automatically by URTSStatUpgrade::ApplyUpgrade(). */
    void AddStatBonus(float HealthBonus, float ManaBonus);

    /** Sets MaximumHealthBonus/MaximumManaBonus on the given actor to match
     *  TotalMaxHealthBonus/TotalMaxManaBonus. Safe to call on actors that lack
     *  health or mana components (no-ops). */
    void ApplyStatUpgradesToActor(AActor* Actor);

    UFUNCTION(BlueprintPure, Category = "RTS") float GetTotalMaxHealthBonus() const { return TotalMaxHealthBonus; }
    UFUNCTION(BlueprintPure, Category = "RTS") float GetTotalMaxManaBonus()   const { return TotalMaxManaBonus; }

    /** Fired on both server and clients when a new upgrade is researched. */
    UPROPERTY(BlueprintAssignable, Category = "RTS")
    FRTSPlayerUpgradeComponentUpgradeResearchedSignature OnUpgradeResearched;

private:
    UPROPERTY(ReplicatedUsing = OnRep_ResearchedUpgrades)
    TArray<TSubclassOf<URTSUpgrade>> ResearchedUpgrades;

    UPROPERTY(Replicated)
    float TotalMaxHealthBonus = 0.f;

    UPROPERTY(Replicated)
    float TotalMaxManaBonus = 0.f;

    UFUNCTION()
    void OnRep_ResearchedUpgrades();
};
