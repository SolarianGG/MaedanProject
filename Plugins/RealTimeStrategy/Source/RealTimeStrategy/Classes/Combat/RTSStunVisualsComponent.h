#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "GameplayTagContainer.h"

#include "RTSStunVisualsComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class UNiagaraComponent;
class UNiagaraSystem;


/** Plays a stun montage and attaches a Niagara effect above the actor while it carries
 *  the Status_Changing_Immobilized tag. Add this to any unit that can be stunned. */
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSStunVisualsComponent : public UActorComponent
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

private:
    /** Montage to play while the actor is stunned. Should have a looping section. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UAnimMontage* StunMontage;

    /** Niagara system to attach above the actor while stunned. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    UNiagaraSystem* StunEffect;

    /** Offset applied when attaching the effect (relative to root or socket). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FVector StunEffectOffset = FVector(0.f, 0.f, 100.f);

    /** Rotation applied when attaching the effect (relative to root or socket). */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FRotator StunEffectRotation = FRotator::ZeroRotator;

    /** Skeletal mesh socket to attach the effect to. Leave empty to use root + StunEffectOffset. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    FName StunEffectSocket;

    bool bIsStunned = false;

    UPROPERTY()
    UNiagaraComponent* ActiveStunEffect = nullptr;

    /** Cached so we can stop the montage when stun ends. */
    UPROPERTY()
    UAnimInstance* CachedAnimInstance = nullptr;

    UFUNCTION()
    void OnTagsChanged(AActor* Actor, FGameplayTagContainer CurrentTags);

    void StartStunVisuals();
    void StopStunVisuals();
};
