#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "Combat/RTSAttackData.h"

#include "RTSPassiveStunComponent.generated.h"

class ARTSProjectile;


/** Adds a percentage chance to stun enemies on each attack, blocking both movement and attacks. */
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSPassiveStunComponent : public UActorComponent
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

private:
    /** Probability of applying a stun on each attack [0..1]. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
    float StunChance = 0.15f;

    /** Duration of the stun effect, in seconds. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0.0f))
    float StunDuration = 1.5f;

    /** Active stun timer handles per stunned actor. Allows re-stun to reset the timer. */
    TMap<TWeakObjectPtr<AActor>, FTimerHandle> ActiveStuns;

    UFUNCTION()
    void HandleAttackUsed(AActor* Actor, const FRTSAttackData& Attack, AActor* Target, ARTSProjectile* Projectile);

    void ApplyStun(AActor* Target);
    void RemoveStun(TWeakObjectPtr<AActor> WeakTarget);
};
