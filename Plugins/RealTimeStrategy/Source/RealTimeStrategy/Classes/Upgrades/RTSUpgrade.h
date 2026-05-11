#pragma once

#include "CoreMinimal.h"

#include "UObject/Object.h"

#include "RTSUpgrade.generated.h"


class AController;
class UTexture2D;


/** Base class for player-wide upgrades. Used as CDOs, like URTSAbility.
 *  Subclass and override ApplyUpgrade() to implement custom upgrade effects. */
UCLASS(BlueprintType, Blueprintable, Abstract)
class REALTIMESTRATEGY_API URTSUpgrade : public UObject
{
    GENERATED_BODY()

public:
    /** Applies this upgrade's effect to the owning player. Server-only. Subclasses must call Super. */
    virtual void ApplyUpgrade(AController* PlayerController);

    /** Whether this upgrade can currently be researched by the player. */
    virtual bool CanResearch(AController* PlayerController) const;

    UFUNCTION(BlueprintPure) FText GetUpgradeName() const { return UpgradeName; }
    UFUNCTION(BlueprintPure) FText GetUpgradeDescription() const { return UpgradeDescription; }
    UFUNCTION(BlueprintPure) UTexture2D* GetUpgradeIcon() const { return UpgradeIcon; }

protected:
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FText UpgradeName;

    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FText UpgradeDescription;

    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UTexture2D* UpgradeIcon;
};
