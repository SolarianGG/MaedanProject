#include "Combat/RTSPassiveStunComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

#include "RTSLog.h"
#include "Abilities/RTSAbilityData.h"
#include "Abilities/RTSAbilitySystemComponent.h"
#include "Abilities/RTSPassiveAbility.h"
#include "Combat/RTSAttackComponent.h"
#include "Libraries/RTSGameplayTagLibrary.h"
#include "Upgrades/RTSPlayerUpgradeComponent.h"


void URTSPassiveStunComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    URTSAttackComponent* AttackComponent = Owner->FindComponentByClass<URTSAttackComponent>();
    if (IsValid(AttackComponent))
    {
        AttackComponent->OnAttackUsed.AddDynamic(this, &URTSPassiveStunComponent::HandleAttackUsed);
    }
}

void URTSPassiveStunComponent::HandleAttackUsed(AActor* Actor, const FRTSAttackData& Attack, AActor* Target, ARTSProjectile* Projectile)
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    if (!IsValid(Target))
    {
        return;
    }

    if (!URTSGameplayTagLibrary::HasGameplayTag(Target, URTSGameplayTagLibrary::Status_Changing_Alive()))
    {
        return;
    }

    // Find the passive ability slot and check its RequiredUpgrade.
    URTSAbilitySystemComponent* AbilitySystem = Owner->FindComponentByClass<URTSAbilitySystemComponent>();
    if (IsValid(AbilitySystem))
    {
        for (const FRTSAbilityData& AbilityData : AbilitySystem->GetAbilities())
        {
            if (!AbilityData.AbilityClass || !AbilityData.RequiredUpgrade)
            {
                continue;
            }

            if (!AbilityData.AbilityClass->IsChildOf(URTSPassiveAbility::StaticClass()))
            {
                continue;
            }

            APawn* OwnerPawn = Cast<APawn>(Owner);
            AController* Controller = IsValid(OwnerPawn) ? OwnerPawn->GetController() : nullptr;
            URTSPlayerUpgradeComponent* UpgradeComp = IsValid(Controller)
                ? Controller->FindComponentByClass<URTSPlayerUpgradeComponent>()
                : nullptr;

            if (!IsValid(UpgradeComp) || !UpgradeComp->HasUpgrade(AbilityData.RequiredUpgrade))
            {
                return;
            }

            break;
        }
    }

    if (FMath::FRandRange(0.0f, 1.0f) < StunChance)
    {
        ApplyStun(Target);
    }
}

void URTSPassiveStunComponent::ApplyStun(AActor* Target)
{
    URTSGameplayTagLibrary::AddGameplayTag(Target, URTSGameplayTagLibrary::Status_Changing_Immobilized());
    URTSGameplayTagLibrary::AddGameplayTag(Target, URTSGameplayTagLibrary::Status_Changing_Unarmed());

    TWeakObjectPtr<AActor> WeakTarget(Target);
    FTimerHandle& Handle = ActiveStuns.FindOrAdd(WeakTarget);

    FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this, WeakTarget]()
    {
        RemoveStun(WeakTarget);
    });

    GetWorld()->GetTimerManager().SetTimer(Handle, Delegate, StunDuration, false);

    UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s stunned %s for %.1fs."),
        *GetOwner()->GetName(), *Target->GetName(), StunDuration);
}

void URTSPassiveStunComponent::RemoveStun(TWeakObjectPtr<AActor> WeakTarget)
{
    if (WeakTarget.IsValid())
    {
        URTSGameplayTagLibrary::RemoveGameplayTag(WeakTarget.Get(), URTSGameplayTagLibrary::Status_Changing_Immobilized());
        URTSGameplayTagLibrary::RemoveGameplayTag(WeakTarget.Get(), URTSGameplayTagLibrary::Status_Changing_Unarmed());
    }

    ActiveStuns.Remove(WeakTarget);
}
