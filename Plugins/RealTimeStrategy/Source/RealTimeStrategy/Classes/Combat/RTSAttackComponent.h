#pragma once

#include "CoreMinimal.h"

#include "RTSActorComponent.h"

#include "Combat/RTSAttackData.h"

#include "RTSAttackComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSAttackComponentCooldownReadySignature, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRTSAttackComponentAttackedUsedSignature, AActor*, Actor, const FRTSAttackData&, Attack, AActor*, Target, ARTSProjectile*, Projectile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSAttackComponentAttackLandedSignature, AActor*, Attacker, AActor*, Target);


/**
* Adds one or more attacks to the actor.
* These can also be used for healing.
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSAttackComponent : public URTSActorComponent
{
	GENERATED_BODY()

public:
	URTSAttackComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/** Uses the passed attack on the specified target and starts the cooldown timer. */
	UFUNCTION(BlueprintCallable)
	void UseAttack(int32 AttackIndex, AActor* Target);

	/** Spawns the projectile stored by the last UseAttack call. Called by RTSAnimNotify_FireProjectile. */
	UFUNCTION(BlueprintCallable)
	void FirePendingProjectile();

	/** Applies melee damage stored by the last UseAttack call. Called by RTSAnimNotify_MeleeAttack. */
	UFUNCTION(BlueprintCallable)
	void ApplyPendingMeleeAttack();

    /** Gets the radius in which the actor will automatically select and attack targets, in cm. */
    UFUNCTION(BlueprintPure)
    float GetAcquisitionRadius() const;

    /** Gets the radius around the home location of the actor it won't leave when automatically attacking targets, in cm. */
    UFUNCTION(BlueprintPure)
    float GetChaseRadius() const;

    /** Gets the attacks available for the actor. Different attacks might be used at different ranges, or against different types of targets. */
    UFUNCTION(BlueprintPure)
    TArray<FRTSAttackData> GetAttacks() const;

    /** Gets the time before the next attack can be used, in seconds. This is shared between attacks.*/
    UFUNCTION(BlueprintPure)
    float GetRemainingCooldown() const;


	/** Event when the attack cooldown has expired. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSAttackComponentCooldownReadySignature OnCooldownReady;

	/** Event when an actor has used an attack. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSAttackComponentAttackedUsedSignature OnAttackUsed;

	/** Event when an attack actually lands on the target (anim notify hit or direct melee damage). */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSAttackComponentAttackLandedSignature OnAttackLanded;

private:
    /** Radius in which the actor will automatically select and attack targets, in cm. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float AcquisitionRadius;

    /** Radius around the home location of the actor it won't leave when automatically attacking targets, in cm. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float ChaseRadius;

    /** Attacks available for the actor. Different attacks might be used at different ranges, or against different types of targets. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TArray<FRTSAttackData> Attacks;

    /** Time before the next attack can be used, in seconds. This is shared between attacks.*/
    float RemainingCooldown;

    struct FPendingProjectile
    {
        TWeakObjectPtr<AActor> Target;
        float Damage = 0.f;
        TSubclassOf<UDamageType> DamageType;
        TSubclassOf<ARTSProjectile> ProjectileClass;
        FName SpawnSocket;
        TWeakObjectPtr<AController> Instigator;
    };

    FPendingProjectile PendingProjectile;
    FTimerHandle ProjectileSpawnTimerHandle;

    void SpawnPendingProjectile();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAttackMontage(UAnimMontage* Montage, float PlayRate);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopAttackMontage();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastNotifyAttackUsed(AActor* InActor, AActor* InTarget);

	/** The montage currently playing as part of an attack. Null when no attack is in progress. */
	UPROPERTY()
	UAnimMontage* CurrentAttackMontage = nullptr;
};
