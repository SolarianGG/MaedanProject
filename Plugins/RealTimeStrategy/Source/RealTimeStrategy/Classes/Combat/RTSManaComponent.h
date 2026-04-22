#pragma once

#include "CoreMinimal.h"

#include "RTSActorComponent.h"

#include "RTSManaComponent.generated.h"


class AActor;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTSManaComponentManaChangedSignature, AActor*, Actor, float, OldMana, float, NewMana);


/**
* Adds a mana resource to the actor, used for casting abilities.
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSManaComponent : public URTSActorComponent
{
	GENERATED_BODY()

public:
	URTSManaComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void BeginPlay() override;


    /** Gets the maximum mana of the actor. */
    UFUNCTION(BlueprintPure)
    float GetMaximumMana() const;

    /** Gets the current mana of the actor. */
    UFUNCTION(BlueprintPure)
    float GetCurrentMana() const;

    /** Sets the current mana of the actor directly. */
    void SetCurrentMana(float NewMana);

    /** Attempts to consume the specified amount of mana. Returns true if successful (enough mana available). Server-authority only. */
    UFUNCTION(BlueprintCallable)
    bool ConsumeMana(float Amount);


    /** Event when the current mana of the actor has changed. */
    virtual void NotifyOnManaChanged(AActor* Actor, float OldMana, float NewMana);


	/** Event when the current mana of the actor has changed. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSManaComponentManaChangedSignature OnManaChanged;

private:
    /** Maximum mana of the actor. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0))
    float MaximumMana;

    /** Whether the actor is allowed to periodically regenerate mana. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    bool bRegenerateMana;

    /** Mana restored for the actor, per second. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS", meta = (ClampMin = 0, EditCondition = "bRegenerateMana"))
    float ManaRegenerationRate;

    /** Current mana of the actor. */
    UPROPERTY(ReplicatedUsing=ReceivedCurrentMana)
    float CurrentMana;

    /** Timer for ticking mana regeneration. */
    FTimerHandle ManaRegenerationTimer;

    UFUNCTION()
    void OnManaRegenerationTimerElapsed();

    UFUNCTION()
    void ReceivedCurrentMana(float OldMana);
};
