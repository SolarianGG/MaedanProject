#include "Combat/RTSManaBarWidgetComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "Combat/RTSManaComponent.h"


void URTSManaBarWidgetComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();

    if (!IsValid(Owner))
    {
        return;
    }

    ManaComponent = Owner->FindComponentByClass<URTSManaComponent>();

    if (!IsValid(ManaComponent))
    {
        return;
    }

    ManaComponent->OnManaChanged.AddDynamic(this, &URTSManaBarWidgetComponent::OnManaChanged);

    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &URTSManaBarWidgetComponent::BroadcastInitialMana);
}

void URTSManaBarWidgetComponent::BroadcastInitialMana()
{
    if (!IsValid(ManaComponent))
    {
        return;
    }

    UpdateManaBar(ManaComponent->GetCurrentMana() / ManaComponent->GetMaximumMana());
    UpdateManaBarValues(ManaComponent->GetCurrentMana(), ManaComponent->GetMaximumMana());
}

void URTSManaBarWidgetComponent::OnManaChanged(AActor* Actor, float OldMana, float NewMana)
{
    UpdateManaBar(ManaComponent->GetCurrentMana() / ManaComponent->GetMaximumMana());
    UpdateManaBarValues(ManaComponent->GetCurrentMana(), ManaComponent->GetMaximumMana());
}
