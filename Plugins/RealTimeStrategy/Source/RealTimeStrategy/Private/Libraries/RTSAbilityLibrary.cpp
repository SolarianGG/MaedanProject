#include "Libraries/RTSAbilityLibrary.h"

#include "GameFramework/Actor.h"

#include "Abilities/RTSAbility.h"
#include "Abilities/RTSAbilitySystemComponent.h"
#include "Combat/RTSManaComponent.h"


bool URTSAbilityLibrary::CanUseAbility(const AActor* Actor, int32 AbilityIndex)
{
    if (!IsValid(Actor))
    {
        return false;
    }

    URTSAbilitySystemComponent* AbilitySystem = Actor->FindComponentByClass<URTSAbilitySystemComponent>();

    if (!IsValid(AbilitySystem))
    {
        return false;
    }

    return AbilitySystem->CanUseAbility(AbilityIndex);
}

TArray<FRTSAbilityData> URTSAbilityLibrary::GetAbilities(const AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return TArray<FRTSAbilityData>();
    }

    URTSAbilitySystemComponent* AbilitySystem = Actor->FindComponentByClass<URTSAbilitySystemComponent>();

    if (!IsValid(AbilitySystem))
    {
        return TArray<FRTSAbilityData>();
    }

    return AbilitySystem->GetAbilities();
}

float URTSAbilityLibrary::GetAbilityManaCost(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return 0.0f;
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetManaCost();
}

float URTSAbilityLibrary::GetAbilityCooldown(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return 0.0f;
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetCooldown();
}

float URTSAbilityLibrary::GetAbilityRange(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return 0.0f;
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetRange();
}

ERTSAbilityTargetType URTSAbilityLibrary::GetAbilityTargetType(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return ERTSAbilityTargetType::ABILITYTARGET_None;
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetTargetType();
}

UTexture2D* URTSAbilityLibrary::GetAbilityIcon(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return nullptr;
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetAbilityIcon();
}

FText URTSAbilityLibrary::GetAbilityName(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return FText::GetEmpty();
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetAbilityName();
}

FText URTSAbilityLibrary::GetAbilityDescription(TSubclassOf<URTSAbility> AbilityClass)
{
    if (!AbilityClass)
    {
        return FText::GetEmpty();
    }

    return AbilityClass->GetDefaultObject<URTSAbility>()->GetAbilityDescription();
}

float URTSAbilityLibrary::GetCurrentMana(const AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return 0.0f;
    }

    URTSManaComponent* ManaComponent = Actor->FindComponentByClass<URTSManaComponent>();

    if (!IsValid(ManaComponent))
    {
        return 0.0f;
    }

    return ManaComponent->GetCurrentMana();
}

float URTSAbilityLibrary::GetMaxMana(const AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return 0.0f;
    }

    URTSManaComponent* ManaComponent = Actor->FindComponentByClass<URTSManaComponent>();

    if (!IsValid(ManaComponent))
    {
        return 0.0f;
    }

    return ManaComponent->GetMaximumMana();
}
