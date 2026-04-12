#pragma once

#include "CoreMinimal.h"

#include "RTSAbilityTargetType.generated.h"


/** Targeting mode for an ability. */
UENUM(BlueprintType)
enum class ERTSAbilityTargetType : uint8
{
    /** Ability requires no target (toggle or passive). */
    ABILITYTARGET_None,

    /** Ability requires a target actor. */
    ABILITYTARGET_Actor,

    /** Ability requires a target location. */
    ABILITYTARGET_Location,

    /** Ability is applied to the caster. */
    ABILITYTARGET_Self
};
