#include "Combat/RTSManaComponent.h"

#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

#include "RTSLog.h"
#include "Libraries/RTSGameplayTagLibrary.h"


URTSManaComponent::URTSManaComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	// Set reasonable default values.
	MaximumMana = 100.0f;
    bRegenerateMana = false;
    ManaRegenerationRate = 1.0f;

    InitialGameplayTags.AddTag(URTSGameplayTagLibrary::Status_Permanent_HasMana());
}

void URTSManaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URTSManaComponent, CurrentMana);
}

void URTSManaComponent::BeginPlay()
{
    Super::BeginPlay();

    // Set initial mana.
    CurrentMana = MaximumMana;

    AActor* Owner = GetOwner();

    if (!IsValid(Owner))
    {
        return;
    }

    if (bRegenerateMana && Owner->HasAuthority())
    {
        // Set up mana regeneration timer.
        Owner->GetWorldTimerManager().SetTimer(
            ManaRegenerationTimer, this, &URTSManaComponent::OnManaRegenerationTimerElapsed, 1.0f, true);
    }
}

float URTSManaComponent::GetMaximumMana() const
{
    return MaximumMana;
}

float URTSManaComponent::GetCurrentMana() const
{
    return CurrentMana;
}

void URTSManaComponent::SetCurrentMana(float NewMana)
{
    float OldMana = CurrentMana;
    CurrentMana = FMath::Clamp(NewMana, 0.0f, MaximumMana);

    // Notify listeners.
    AActor* Owner = GetOwner();
    NotifyOnManaChanged(Owner, OldMana, CurrentMana);
}

bool URTSManaComponent::ConsumeMana(float Amount)
{
    AActor* Owner = GetOwner();

    if (!IsValid(Owner) || !Owner->HasAuthority())
    {
        return false;
    }

    if (CurrentMana < Amount)
    {
        return false;
    }

    SetCurrentMana(CurrentMana - Amount);
    return true;
}

void URTSManaComponent::NotifyOnManaChanged(AActor* Actor, float OldMana, float NewMana)
{
    OnManaChanged.Broadcast(Actor, OldMana, NewMana);
}

void URTSManaComponent::OnManaRegenerationTimerElapsed()
{
    if (CurrentMana >= MaximumMana)
    {
        return;
    }

    float NewMana = FMath::Clamp(CurrentMana + ManaRegenerationRate, 0.0f, MaximumMana);
    SetCurrentMana(NewMana);
}

void URTSManaComponent::ReceivedCurrentMana(float OldMana)
{
    NotifyOnManaChanged(GetOwner(), OldMana, CurrentMana);
}
