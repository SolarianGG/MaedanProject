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


    /** Gets the maximum mana of the actor (base + upgrade bonus). */
    UFUNCTION(BlueprintPure)
    float GetMaximumMana() const;

    /** Gets the current mana of the actor. */
    UFUNCTION(BlueprintPure)
    float GetCurrentMana() const;

    /** Sets the current mana of the actor directly. */
    void SetCurrentMana(float NewMana);

    /** Sets the flat bonus added on top of MaximumMana by upgrades.
     *  Pass the new total bonus (absolute, not delta). Server-only.
     *  If bScaleCurrentMana, current mana is scaled proportionally. */
    void SetMaximumManaBonus(float NewBonus, bool bScaleCurrentMana = true);

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

    /** Flat Mana bonus from upgrades. Replicated so clients display correct mana bars. */
    UPROPERTY(Replicated)
    float MaximumManaBonus = 0.f;

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
