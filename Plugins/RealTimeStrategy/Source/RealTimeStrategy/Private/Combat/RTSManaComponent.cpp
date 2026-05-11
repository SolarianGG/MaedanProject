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
	DOREPLIFETIME(URTSManaComponent, MaximumManaBonus);
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
    return MaximumMana + MaximumManaBonus;
}

void URTSManaComponent::SetMaximumManaBonus(float NewBonus, bool bScaleCurrentMana /*= true*/)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    const float OldMax = GetMaximumMana();
    MaximumManaBonus = FMath::Max(NewBonus, 0.f);
    const float NewMax = GetMaximumMana();

    if (bScaleCurrentMana && OldMax > KINDA_SMALL_NUMBER)
    {
        SetCurrentMana(CurrentMana * (NewMax / OldMax));
    }
    else
    {
        SetCurrentMana(FMath::Min(CurrentMana, NewMax));
    }
}

float URTSManaComponent::GetCurrentMana() const
{
    return CurrentMana;
}

void URTSManaComponent::SetCurrentMana(float NewMana)
{
    float OldMana = CurrentMana;
    CurrentMana = FMath::Clamp(NewMana, 0.0f, GetMaximumMana());

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
    const float MaxMana = GetMaximumMana();

    if (CurrentMana >= MaxMana)
    {
        return;
    }

    float NewMana = FMath::Clamp(CurrentMana + ManaRegenerationRate, 0.0f, MaxMana);
    SetCurrentMana(NewMana);
}

void URTSManaComponent::ReceivedCurrentMana(float OldMana)
{
    NotifyOnManaChanged(GetOwner(), OldMana, CurrentMana);
}
