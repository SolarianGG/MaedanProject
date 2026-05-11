#pragma once

#include "CoreMinimal.h"

#include "UI/RTSActorWidgetComponent.h"

#include "RTSHealthBarWidgetComponent.generated.h"


class URTSHealthComponent;
class URTSManaComponent;


/**
* Adds a health (and optionally mana) bar widget to the actor.
*/
UCLASS(Blueprintable)
class REALTIMESTRATEGY_API URTSHealthBarWidgetComponent : public URTSActorWidgetComponent
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    /** Event when the current health of the actor has changed. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS")
    void UpdateHealthBar(float HealthPercentage);

    /** Event when health changes — passes absolute values for text display. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS")
    void UpdateHealthBarValues(float CurrentHealth, float MaxHealth);

    /** Event when the current mana of the actor has changed. Only fires if actor has URTSManaComponent. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS")
    void UpdateManaBar(float ManaPercentage);

    /** Event when mana changes — passes absolute values for text display. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS")
    void UpdateManaBarValues(float CurrentMana, float MaxMana);


private:
    UPROPERTY()
    URTSHealthComponent* HealthComponent;

    UPROPERTY()
    URTSManaComponent* ManaComponent;

    UFUNCTION()
    void OnHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser);

    UFUNCTION()
    void OnManaChanged(AActor* Actor, float OldMana, float NewMana);

    UFUNCTION()
    void BroadcastInitialValues();
};
