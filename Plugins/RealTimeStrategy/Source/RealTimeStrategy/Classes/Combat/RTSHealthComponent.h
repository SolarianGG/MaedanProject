#pragma once

#include "CoreMinimal.h"

#include "RTSActorComponent.h"

#include "Combat/RTSActorDeathType.h"

#include "RTSHealthComponent.generated.h"


class AActor;
class UAnimMontage;
class USoundCue;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRTSHealthComponentHealthChangedSignature, AActor*, Actor, float, OldHealth, float, NewHealth, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTSHealthComponentKilledSignature, AActor*, Actor, AController*, PreviousOwner, AActor*, DamageCauser);


/**
* Adds health to the actor, e.g. for taking damage and dying.
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSHealthComponent : public URTSActorComponent
{
	GENERATED_BODY()

public:
	URTSHealthComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;


    /** Gets the maximum health of the actor (base + upgrade bonus). */
    UFUNCTION(BlueprintPure)
    float GetMaximumHealth() const;

    /** Gets the current health of the actor. */
    UFUNCTION(BlueprintPure)
    float GetCurrentHealth() const;

    /** Sets the current health of the actor directly. */
    void SetCurrentHealth(float NewHealth, AActor* DamageCauser);

    /** Sets the flat bonus added on top of MaximumHealth by upgrades.
     *  Pass the new total bonus (absolute, not delta). Server-only.
     *  If bScaleCurrentHealth, current HP is scaled proportionally. */
    void SetMaximumHealthBonus(float NewBonus, bool bScaleCurrentHealth = true);

    /** Kills the actor immediately. */
    void KillActor(AActor* DamageCauser = nullptr);

    /** Gets the last time the actor has taken damage. */
    float GetLastTimeDamageTaken() const;


    /** Event when the current health of the actor has changed. */
    virtual void NotifyOnHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser);

    /** Plays the death montage on all clients. */
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayDeathMontage();


	/** Event when the current health of the actor has changed. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSHealthComponentHealthChangedSignature OnHealthChanged;

	/** Event when the actor has been killed. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSHealthComponentKilledSignature OnKilled;

private:
    /** Maximum health of the actor. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float MaximumHealth;

    /** Whether the actor is allowed to periodically regenerate health. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    bool bRegenerateHealth;

    /** Health restored for the actor, per second. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = "bRegenerateHealth"))
    float HealthRegenerationRate;

    /** How to handle depleted health. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    ERTSActorDeathType ActorDeathType;

    /** Sound to play when the actor is killed. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    USoundCue* DeathSound;

    /** Animation montage to play when the actor is killed. For DEATH_Destroy, the actor is destroyed after the montage finishes. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UAnimMontage* DeathMontage;

    /** Flat HP bonus from upgrades. Replicated so clients display correct HP bars. */
    UPROPERTY(Replicated)
    float MaximumHealthBonus = 0.f;

    /** Current health of the actor. */
    UPROPERTY(ReplicatedUsing=ReceivedCurrentHealth)
    float CurrentHealth;

    /** Last time the actor has taken damage. */
    float LastTimeDamageTaken;

    /** Timer for ticking health regeneration. */
    FTimerHandle HealthRegenerationTimer;

    /** Timer for destroying the actor after the death montage finishes. */
    FTimerHandle DeathDestroyTimer;

    UFUNCTION()
    void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

    UFUNCTION()
    void OnHealthRegenerationTimerElapsed();

    UFUNCTION()
    void ReceivedCurrentHealth(float OldHealth);
};
