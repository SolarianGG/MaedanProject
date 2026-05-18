#pragma once

#include "CoreMinimal.h"

#include "InputCoreTypes.h"
#include "UObject/Object.h"

#include "GameFramework/DamageType.h"
#include "Templates/SubclassOf.h"

#include "Abilities/RTSAbilityTargetType.h"
#include "Combat/RTSProjectile.h"
#include "Orders/RTSOrderTagRequirements.h"
#include "Orders/RTSOrderTargetData.h"

#include "RTSAbility.generated.h"


class AActor;
class UAnimMontage;
class UNiagaraSystem;
class UTexture2D;


/** Base class for abilities that can be cast by actors. Used as CDOs like URTSOrder. */
UCLASS(BlueprintType, Blueprintable)
class REALTIMESTRATEGY_API URTSAbility : public UObject
{
    GENERATED_BODY()

public:
    /** Whether the specified caster can activate this ability. */
    virtual bool CanActivateAbility(const AActor* Caster, int32 AbilityIndex) const;

    /** Whether the specified target data is valid for this ability. */
    virtual bool IsValidAbilityTarget(const AActor* Caster, const FRTSOrderTargetData& TargetData) const;

    /** Activates the ability for the specified caster. */
    virtual void ActivateAbility(AActor* Caster, const FRTSOrderTargetData& TargetData) const;


    /** Gets the mana cost of this ability. */
    UFUNCTION(BlueprintPure)
    float GetManaCost() const;

    /** Gets the cooldown of this ability, in seconds. */
    UFUNCTION(BlueprintPure)
    float GetCooldown() const;

    /** Gets the range of this ability, in cm. 0 means unlimited or self-targeted. */
    UFUNCTION(BlueprintPure)
    float GetRange() const;

    /** Gets the target type required by this ability. */
    UFUNCTION(BlueprintPure)
    ERTSAbilityTargetType GetTargetType() const;

    /** Gets the tag requirements for this ability. */
    FRTSOrderTagRequirements GetAbilityTagRequirements() const;

    /** Gets the icon of this ability. */
    UFUNCTION(BlueprintPure)
    UTexture2D* GetAbilityIcon() const;

    /** Gets the name of this ability. */
    UFUNCTION(BlueprintPure)
    FText GetAbilityName() const;

    /** Gets the description of this ability. */
    UFUNCTION(BlueprintPure)
    FText GetAbilityDescription() const;

    /** Gets the damage dealt by this ability. */
    UFUNCTION(BlueprintPure)
    float GetDamage() const;

    /** Gets the animation montage to play when casting. */
    UFUNCTION(BlueprintPure)
    UAnimMontage* GetAbilityMontage() const;

    /** Gets the Niagara effect to spawn at the caster. */
    UFUNCTION(BlueprintPure)
    UNiagaraSystem* GetCasterEffect() const;

    /** Gets the Niagara effect to spawn at the target. */
    UFUNCTION(BlueprintPure)
    UNiagaraSystem* GetTargetEffect() const;

    /** Gets the vertical offset applied to the target effect spawn location. */
    UFUNCTION(BlueprintPure)
    float GetTargetEffectZOffset() const;

    /** Gets the world-space diameter (cm) of the targeting circle. Mirror this to your Niagara effect's Uniform Sprite Size. */
    UFUNCTION(BlueprintPure)
    float GetTargetingCircleSize() const;

    /** Gets the keyboard key that triggers this ability. EKeys::Invalid means no hotkey assigned. */
    UFUNCTION(BlueprintPure)
    FKey GetHotkey() const;

protected:
    /** Mana cost to activate this ability. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float ManaCost;

    /** Time before this ability can be used again, in seconds. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float Cooldown;

    /** Range of this ability, in cm. 0 means unlimited or self-targeted. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float Range;

    /** Target type required by this ability. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    ERTSAbilityTargetType TargetType;

    /** Tag requirements for the caster and target. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FRTSOrderTagRequirements AbilityTagRequirements;

    /** Animation montage to play when casting. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UAnimMontage* AbilityMontage;

    /** Projectile to spawn. If not set, damage is dealt instantly. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TSubclassOf<ARTSProjectile> ProjectileClass;

    /** Damage dealt by this ability. Negative values could mean healing. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    float Damage;

    /** Type of damage caused by this ability. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TSubclassOf<UDamageType> DamageType;

    /** Icon of this ability. Can be shown in the UI. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UTexture2D* AbilityIcon;

    /** Name of this ability. Can be shown in the UI. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FText AbilityName;

    /** Description of this ability. Can be shown in the UI. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FText AbilityDescription;

    /** Niagara effect to spawn at the caster's location when the ability is activated. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|VFX")
    UNiagaraSystem* CasterEffect;

    /** Niagara effect to spawn at the target location (or target actor) when the ability is activated. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|VFX")
    UNiagaraSystem* TargetEffect;

    /** Vertical offset (cm) added to the target effect spawn location. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|VFX")
    float TargetEffectZOffset;

    /** World-space diameter (cm) of the targeting circle decal. Set this to match the Niagara effect's Uniform Sprite Size. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|VFX", meta = (ClampMin = 0))
    float TargetingCircleSize;

    /** Keyboard key that triggers this ability when pressed. EKeys::Invalid means no hotkey. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Input")
    FKey Hotkey;
};
