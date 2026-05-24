#pragma once

#include "CoreMinimal.h"

#include "Abilities/RTSAbility.h"

#include "RTSPassiveAbility.generated.h"


/** Base class for passive abilities. Appears in the ability bar with icon/name/description
 *  but cannot be manually activated by the player. */
UCLASS(BlueprintType, Blueprintable)
class REALTIMESTRATEGY_API URTSPassiveAbility : public URTSAbility
{
    GENERATED_BODY()

public:
    virtual bool CanActivateAbility_Implementation(const AActor* Caster, int32 AbilityIndex) const override;
};
