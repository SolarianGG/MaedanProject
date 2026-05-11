#pragma once

#include "CoreMinimal.h"

#include "UI/RTSActorWidgetComponent.h"

#include "RTSManaBarWidgetComponent.generated.h"


class URTSManaComponent;


/**
* Adds a mana bar widget to the actor.
*/
UCLASS(Blueprintable)
class REALTIMESTRATEGY_API URTSManaBarWidgetComponent : public URTSActorWidgetComponent
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    /** Event when the current mana of the actor has changed. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS")
    void UpdateManaBar(float ManaPercentage);

    /** Event when mana changes — passes absolute values for text display. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS")
    void UpdateManaBarValues(float CurrentMana, float MaxMana);


private:
    UPROPERTY()
    URTSManaComponent* ManaComponent;

    UFUNCTION()
    void OnManaChanged(AActor* Actor, float OldMana, float NewMana);

    UFUNCTION()
    void BroadcastInitialMana();
};
