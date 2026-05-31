#include "Combat/RTSPassiveStunComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerState.h"
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
        AttackComponent->OnAttackLanded.AddDynamic(this, &URTSPassiveStunComponent::HandleAttackLanded);
    }
}

void URTSPassiveStunComponent::HandleAttackLanded(AActor* Attacker, AActor* Target)
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    if (!IsValid(Target))
    {
        UE_LOG(LogRTS, Warning, TEXT("URTSPassiveStunComponent: %s — HandleAttackUsed: Target is invalid, skipping."),
            *Owner->GetName());
        return;
    }

    UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s attacked %s — checking stun conditions."),
        *Owner->GetName(), *Target->GetName());

    if (!URTSGameplayTagLibrary::HasGameplayTag(Target, URTSGameplayTagLibrary::Status_Changing_Alive()))
    {
        UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: Target %s is not alive, skipping stun."),
            *Target->GetName());
        return;
    }

    // Find the passive ability slot and check its RequiredUpgrade.
    // Use URTSOwnerComponent -> ARTSPlayerState -> PlayerController to find URTSPlayerUpgradeComponent,
    // because Pawn->GetController() returns ARTSPawnAIController (the unit AI), not the player controller.
    URTSAbilitySystemComponent* AbilitySystem = Owner->FindComponentByClass<URTSAbilitySystemComponent>();
    if (IsValid(AbilitySystem))
    {
        UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s has AbilitySystem with %d ability slot(s)."),
            *Owner->GetName(), AbilitySystem->GetAbilities().Num());

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

            UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: Found passive ability slot requiring upgrade '%s'."),
                *AbilityData.RequiredUpgrade->GetName());

            URTSOwnerComponent* OwnerComp = Owner->FindComponentByClass<URTSOwnerComponent>();
            ARTSPlayerState* PS = IsValid(OwnerComp) ? OwnerComp->GetPlayerOwner() : nullptr;
            AController* PlayerController = IsValid(PS) ? Cast<AController>(PS->GetOwner()) : nullptr;
            URTSPlayerUpgradeComponent* UpgradeComp = IsValid(PlayerController)
                ? PlayerController->FindComponentByClass<URTSPlayerUpgradeComponent>()
                : nullptr;

            if (!IsValid(UpgradeComp))
            {
                UE_LOG(LogRTS, Warning, TEXT("URTSPassiveStunComponent: %s — URTSPlayerUpgradeComponent not found on player controller (PS=%s, PC=%s). Stun blocked."),
                    *Owner->GetName(),
                    IsValid(PS) ? *PS->GetName() : TEXT("null"),
                    IsValid(PlayerController) ? *PlayerController->GetName() : TEXT("null"));
                return;
            }

            if (!UpgradeComp->HasUpgrade(AbilityData.RequiredUpgrade))
            {
                UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s — upgrade '%s' not yet researched. Stun blocked."),
                    *Owner->GetName(), *AbilityData.RequiredUpgrade->GetName());
                return;
            }

            UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s — upgrade '%s' confirmed. Proceeding to stun roll."),
                *Owner->GetName(), *AbilityData.RequiredUpgrade->GetName());
            break;
        }
    }
    else
    {
        UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s has no AbilitySystem — no upgrade gate, proceeding to stun roll."),
            *Owner->GetName());
    }

    const float Roll = FMath::FRandRange(0.0f, 1.0f);
    UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s stun roll %.2f vs chance %.2f — %s."),
        *Owner->GetName(), Roll, StunChance, Roll < StunChance ? TEXT("HIT") : TEXT("miss"));

    if (Roll < StunChance)
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

    UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: %s stunned %s for %.1fs (Immobilized + Unarmed tags added)."),
        *GetOwner()->GetName(), *Target->GetName(), StunDuration);
}

void URTSPassiveStunComponent::RemoveStun(TWeakObjectPtr<AActor> WeakTarget)
{
    if (WeakTarget.IsValid())
    {
        UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: Stun expired on %s — removing Immobilized + Unarmed tags."),
            *WeakTarget->GetName());
        URTSGameplayTagLibrary::RemoveGameplayTag(WeakTarget.Get(), URTSGameplayTagLibrary::Status_Changing_Immobilized());
        URTSGameplayTagLibrary::RemoveGameplayTag(WeakTarget.Get(), URTSGameplayTagLibrary::Status_Changing_Unarmed());
    }
    else
    {
        UE_LOG(LogRTS, Log, TEXT("URTSPassiveStunComponent: Stun timer expired but target is already destroyed — skipping tag removal."));
    }

    ActiveStuns.Remove(WeakTarget);
}
