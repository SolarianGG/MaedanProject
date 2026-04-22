#pragma once

#include "CoreMinimal.h"

#include "RTSActorComponent.h"

#include "Abilities/RTSAbilityData.h"

#include "RTSAbilitySystemComponent.generated.h"


class AActor;
class UNiagaraSystem;
class URTSManaComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTSAbilitySystemAbilityUsedSignature, AActor*, Actor, int32, AbilityIndex, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSAbilitySystemCooldownReadySignature, AActor*, Actor, int32, AbilityIndex);


/**
* Manages abilities available to the actor.
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSAbilitySystemComponent : public URTSActorComponent
{
	GENERATED_BODY()

public:
	URTSAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


    /** Gets the abilities available for the actor. */
    UFUNCTION(BlueprintPure)
    TArray<FRTSAbilityData> GetAbilities() const;

    /** Gets the remaining cooldown of the specified ability, in seconds. */
    UFUNCTION(BlueprintPure)
    float GetRemainingCooldown(int32 AbilityIndex) const;

    /** Whether the specified ability can be used right now. */
    UFUNCTION(BlueprintPure)
    bool CanUseAbility(int32 AbilityIndex) const;

    /** Uses the specified ability on the target. Server-authority only. */
    UFUNCTION(BlueprintCallable)
    void UseAbility(int32 AbilityIndex, AActor* TargetActor, FVector TargetLocation);


	/** Event when an ability has been used. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSAbilitySystemAbilityUsedSignature OnAbilityUsed;

	/** Event when an ability cooldown has expired. */
	UPROPERTY(BlueprintAssignable, Category = "RTS")
	FRTSAbilitySystemCooldownReadySignature OnAbilityCooldownReady;

private:
    /** Abilities available for the actor. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TArray<FRTSAbilityData> Abilities;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAbilityMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastNotifyAbilityUsed(AActor* InActor, int32 InAbilityIndex, AActor* InTarget);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSpawnAbilityEffects(UNiagaraSystem* CasterEffect, UNiagaraSystem* TargetEffect, FVector CasterLocation, FVector TargetLocation, FRotator CasterRotation);
};
