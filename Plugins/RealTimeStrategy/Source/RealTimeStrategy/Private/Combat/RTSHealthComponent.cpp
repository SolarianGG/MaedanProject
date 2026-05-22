#include "Combat/RTSHealthComponent.h"

#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

#include "RTSGameMode.h"
#include "RTSGameplayTagsComponent.h"
#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerState.h"
#include "Libraries/RTSGameplayLibrary.h"
#include "Libraries/RTSGameplayTagLibrary.h"


URTSHealthComponent::URTSHealthComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	// Set reasonable default values.
	MaximumHealth = 100.0f;
    bRegenerateHealth = false;
    ActorDeathType = ERTSActorDeathType::DEATH_Destroy;

    InitialGameplayTags.AddTag(URTSGameplayTagLibrary::Status_Changing_Alive());
}

void URTSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URTSHealthComponent, CurrentHealth);
	DOREPLIFETIME(URTSHealthComponent, MaximumHealthBonus);
}

void URTSHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // Set initial health.
    CurrentHealth = MaximumHealth;

    // Register for events.
    AActor* Owner = GetOwner();

    if (!IsValid(Owner))
    {
        return;
    }

    Owner->OnTakeAnyDamage.AddDynamic(this, &URTSHealthComponent::OnTakeAnyDamage);

    if (bRegenerateHealth && Owner->HasAuthority())
    {
        // Set up health regeneration timer.
        Owner->GetWorldTimerManager().SetTimer(
            HealthRegenerationTimer, this, &URTSHealthComponent::OnHealthRegenerationTimerElapsed, 1.0f, true);
    }

    LastTimeDamageTaken = 0.0f;
}

float URTSHealthComponent::GetMaximumHealth() const
{
    return MaximumHealth + MaximumHealthBonus;
}

void URTSHealthComponent::SetMaximumHealthBonus(float NewBonus, bool bScaleCurrentHealth /*= true*/)
{
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    const float OldMax = GetMaximumHealth();
    MaximumHealthBonus = FMath::Max(NewBonus, 0.f);
    const float NewMax = GetMaximumHealth();

    if (bScaleCurrentHealth && OldMax > KINDA_SMALL_NUMBER)
    {
        SetCurrentHealth(FMath::Clamp(CurrentHealth * (NewMax / OldMax), 0.f, NewMax), nullptr);
    }
    else
    {
        SetCurrentHealth(FMath::Clamp(CurrentHealth, 0.f, NewMax), nullptr);
    }
}

float URTSHealthComponent::GetCurrentHealth() const
{
    return CurrentHealth;
}

void URTSHealthComponent::SetCurrentHealth(float NewHealth, AActor* DamageCauser)
{
    float OldHealth = CurrentHealth;
    CurrentHealth = NewHealth;

    // Notify listeners.
    AActor* Owner = GetOwner();

    NotifyOnHealthChanged(Owner, OldHealth, NewHealth, DamageCauser);

    if (GetWorld() && OldHealth > NewHealth)
    {
        LastTimeDamageTaken = GetWorld()->GetRealTimeSeconds();
    }

    // Check if we've just died (guard against re-entry while death montage plays).
    if (CurrentHealth <= 0 && OldHealth > 0)
    {
        UE_LOG(LogRTS, Log, TEXT("Actor %s has been killed."), *Owner->GetName());

        // Resolve owning controller: prefer URTSOwnerComponent, fall back to direct owner.
        AController* OwningPlayer = nullptr;
        URTSOwnerComponent* OwnerComponent = Owner->FindComponentByClass<URTSOwnerComponent>();
        if (IsValid(OwnerComponent))
        {
            ARTSPlayerState* PlayerState = OwnerComponent->GetPlayerOwner();
            if (IsValid(PlayerState))
            {
                OwningPlayer = Cast<AController>(PlayerState->GetOwner());
            }
        }
        if (!IsValid(OwningPlayer))
        {
            OwningPlayer = Cast<AController>(Owner->GetOwner());
        }

        // Remove Alive tag.
        URTSGameplayTagLibrary::RemoveGameplayTag(Owner, URTSGameplayTagLibrary::Status_Changing_Alive());

        // Notify listeners.
        OnKilled.Broadcast(Owner, OwningPlayer, DamageCauser);

        // Notify game mode before destroying so defeat condition check can exclude this actor.
        ARTSGameMode* GameMode = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GameMode != nullptr)
        {
            GameMode->NotifyOnActorKilled(Owner, OwningPlayer);
        }

        // Stop or destroy actor.
        switch (ActorDeathType)
        {
        case ERTSActorDeathType::DEATH_DoNothing:
            URTSGameplayLibrary::StopGameplayFor(Owner);
            break;

        case ERTSActorDeathType::DEATH_StopGameplay:
            URTSGameplayLibrary::StopGameplayFor(Owner);
            if (IsValid(DeathMontage))
            {
                MulticastPlayDeathMontage();
            }
            break;

        case ERTSActorDeathType::DEATH_Destroy:
            if (IsValid(DeathMontage))
            {
                // Disable interaction immediately, play montage, then destroy after it finishes.
                URTSGameplayLibrary::StopGameplayFor(Owner);
                MulticastPlayDeathMontage();
                Owner->GetWorldTimerManager().SetTimer(
                    DeathDestroyTimer,
                    [WeakOwner = TWeakObjectPtr<AActor>(Owner)]()
                    {
                        if (WeakOwner.IsValid())
                        {
                            WeakOwner->Destroy();
                        }
                    },
                    DeathMontage->GetPlayLength(),
                    false);
            }
            else
            {
                Owner->Destroy();
            }
            break;
        }
    }
}

void URTSHealthComponent::KillActor(AActor* DamageCauser /*= nullptr*/)
{
    SetCurrentHealth(0.0f, DamageCauser);
}

float URTSHealthComponent::GetLastTimeDamageTaken() const
{
    return LastTimeDamageTaken;
}

void URTSHealthComponent::NotifyOnHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser)
{
    // Notify listeners.
    OnHealthChanged.Broadcast(Actor, OldHealth, NewHealth, DamageCauser);

    // Play sound — guard OldHealth so this fires exactly once on the alive→dead transition.
    if (NewHealth <= 0.0f && OldHealth > 0.0f && IsValid(DeathSound))
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound,
            Actor->GetActorLocation(), Actor->GetActorRotation());
    }
}

void URTSHealthComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
    // Only the server should modify health; clients receive the authoritative value via replication.
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    if (CurrentHealth <= 0)
    {
        return;
    }

    SetCurrentHealth(CurrentHealth - Damage, DamageCauser);
}

void URTSHealthComponent::OnHealthRegenerationTimerElapsed()
{
    const float MaxHealth = GetMaximumHealth();

    if (CurrentHealth >= MaxHealth)
    {
        return;
    }

    float NewHealth = FMath::Clamp(CurrentHealth + HealthRegenerationRate, 0.0f, MaxHealth);
    SetCurrentHealth(NewHealth, GetOwner());
}

void URTSHealthComponent::ReceivedCurrentHealth(float OldHealth)
{
    NotifyOnHealthChanged(GetOwner(), OldHealth, CurrentHealth, nullptr);
}

void URTSHealthComponent::MulticastPlayDeathMontage_Implementation()
{
    if (!IsValid(DeathMontage))
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
    if (Mesh == nullptr)
    {
        return;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (AnimInstance == nullptr)
    {
        return;
    }

    AnimInstance->Montage_Play(DeathMontage);
}
