#pragma once

#include "CoreMinimal.h"

#include "Engine/EngineTypes.h"
#include "Templates/SubclassOf.h"

#include "Abilities/RTSAbility.h"

#include "RTSDoTAreaAbility.generated.h"


class AActor;
class ARTSDamageAreaActor;


/**
 * Ability that spawns a persistent damaging zone at a target location.
 * The zone deals DoTDamagePerTick damage every DoTTickInterval seconds to
 * hostile actors within DoTRadius for DoTDuration seconds.
 */
UCLASS(BlueprintType, Blueprintable)
class REALTIMESTRATEGY_API URTSDoTAreaAbility : public URTSAbility
{
    GENERATED_BODY()

public:
    virtual bool IsValidAbilityTarget(const AActor* Caster, const FRTSOrderTargetData& TargetData) const override;
    virtual void ActivateAbility(AActor* Caster, const FRTSOrderTargetData& TargetData) const override;

protected:
    /** Radius of the spawned zone, in cm. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT", meta = (ClampMin = 0))
    float DoTRadius = 300.f;

    /** Total lifetime of the zone, in seconds. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT", meta = (ClampMin = 0))
    float DoTDuration = 5.f;

    /** Interval between damage ticks, in seconds. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT", meta = (ClampMin = 0.05))
    float DoTTickInterval = 1.f;

    /** Damage applied to each enemy in radius per tick. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT")
    float DoTDamagePerTick = 5.f;

    /** Class of the persistent zone actor to spawn (BP child of ARTSDamageAreaActor with VFX configured). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT")
    TSubclassOf<ARTSDamageAreaActor> AreaActorClass;

    /** Object types to query when overlapping for damage targets in the zone. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT")
    TArray<TEnumAsByte<EObjectTypeQuery>> AreaObjectTypeFilter;

    /** Optional class filter for damage targets in the zone (e.g. APawn). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|DoT")
    TSubclassOf<AActor> AreaClassFilter;
};
