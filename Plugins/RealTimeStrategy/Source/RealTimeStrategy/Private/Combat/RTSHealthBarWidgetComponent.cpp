#include "Combat/RTSHealthBarWidgetComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "Combat/RTSHealthComponent.h"
#include "Combat/RTSManaComponent.h"


void URTSHealthBarWidgetComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();

    if (!IsValid(Owner))
    {
        return;
    }

    HealthComponent = Owner->FindComponentByClass<URTSHealthComponent>();

    if (!IsValid(HealthComponent))
    {
        return;
    }

    HealthComponent->OnHealthChanged.AddDynamic(this, &URTSHealthBarWidgetComponent::OnHealthChanged);

    // Mana is optional — silently skip if actor has no mana component.
    ManaComponent = Owner->FindComponentByClass<URTSManaComponent>();
    if (IsValid(ManaComponent))
    {
        ManaComponent->OnManaChanged.AddDynamic(this, &URTSHealthBarWidgetComponent::OnManaChanged);
    }

    // Defer initial update one tick so all components finish their own BeginPlay first.
    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &URTSHealthBarWidgetComponent::BroadcastInitialValues);
}

void URTSHealthBarWidgetComponent::BroadcastInitialValues()
{
    if (IsValid(HealthComponent))
    {
        UpdateHealthBar(HealthComponent->GetCurrentHealth() / HealthComponent->GetMaximumHealth());
        UpdateHealthBarValues(HealthComponent->GetCurrentHealth(), HealthComponent->GetMaximumHealth());
    }

    if (IsValid(ManaComponent) && ManaComponent->GetMaximumMana() > 0.0f)
    {
        UpdateManaBar(ManaComponent->GetCurrentMana() / ManaComponent->GetMaximumMana());
        UpdateManaBarValues(ManaComponent->GetCurrentMana(), ManaComponent->GetMaximumMana());
    }
}

void URTSHealthBarWidgetComponent::OnHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser)
{
    UpdateHealthBar(HealthComponent->GetCurrentHealth() / HealthComponent->GetMaximumHealth());
    UpdateHealthBarValues(HealthComponent->GetCurrentHealth(), HealthComponent->GetMaximumHealth());
}

void URTSHealthBarWidgetComponent::OnManaChanged(AActor* Actor, float OldMana, float NewMana)
{
    if (ManaComponent->GetMaximumMana() > 0.0f)
    {
        UpdateManaBar(ManaComponent->GetCurrentMana() / ManaComponent->GetMaximumMana());
        UpdateManaBarValues(ManaComponent->GetCurrentMana(), ManaComponent->GetMaximumMana());
    }
}
