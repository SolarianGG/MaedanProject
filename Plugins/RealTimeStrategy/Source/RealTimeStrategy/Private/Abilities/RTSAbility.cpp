#include "Abilities/RTSAbility.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "RTSLog.h"
#include "Combat/RTSProjectile.h"
#include "Libraries/RTSGameplayTagLibrary.h"


bool URTSAbility::CanActivateAbility_Implementation(const AActor* Caster, int32 AbilityIndex) const
{
    return true;
}

bool URTSAbility::IsValidAbilityTarget(const AActor* Caster, const FRTSOrderTargetData& TargetData) const
{
    if (TargetType == ERTSAbilityTargetType::ABILITYTARGET_Actor)
    {
        if (!IsValid(TargetData.Actor))
        {
            return false;
        }

        // Check target tag requirements.
        if (!URTSGameplayTagLibrary::MeetsTagRequirements(
            TargetData.TargetTags,
            AbilityTagRequirements.TargetRequiredTags,
            AbilityTagRequirements.TargetBlockedTags))
        {
            return false;
        }
    }

    return true;
}

void URTSAbility::ActivateAbility(AActor* Caster, const FRTSOrderTargetData& TargetData) const
{
    if (!IsValid(Caster))
    {
        return;
    }

    APawn* CasterPawn = Cast<APawn>(Caster);
    AController* CasterController = Cast<AController>(Caster->GetOwner());

    UE_LOG(LogRTS, Log, TEXT("Actor %s activates ability %s."), *Caster->GetName(), *GetName());

    // Copy TSubclassOf members to local non-const variables for API compatibility.
    TSubclassOf<ARTSProjectile> LocalProjectileClass = ProjectileClass;
    TSubclassOf<UDamageType> LocalDamageType = DamageType;

    if (LocalProjectileClass != nullptr && IsValid(TargetData.Actor))
    {
        // Fire projectile.
        FVector SpawnLocation = Caster->GetActorLocation();
        FRotator SpawnRotation = Caster->GetActorRotation();
        FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);

        FActorSpawnParameters SpawnInfo;
        SpawnInfo.Instigator = CasterPawn;
        SpawnInfo.ObjectFlags |= RF_Transient;

        ARTSProjectile* SpawnedProjectile = Caster->GetWorld()->SpawnActor<ARTSProjectile>(
            LocalProjectileClass, SpawnTransform, SpawnInfo);

        if (SpawnedProjectile)
        {
            SpawnedProjectile->FireAt(
                TargetData.Actor,
                Damage,
                LocalDamageType,
                CasterController,
                Caster);
        }
    }
    else if (Damage != 0.0f && IsValid(TargetData.Actor))
    {
        // Deal damage immediately.
        TargetData.Actor->TakeDamage(Damage, FDamageEvent(LocalDamageType), CasterController, Caster);
    }
}

float URTSAbility::GetManaCost() const
{
    return ManaCost;
}

float URTSAbility::GetCooldown() const
{
    return Cooldown;
}

float URTSAbility::GetRange() const
{
    return Range;
}

ERTSAbilityTargetType URTSAbility::GetTargetType() const
{
    return TargetType;
}

FRTSOrderTagRequirements URTSAbility::GetAbilityTagRequirements() const
{
    return AbilityTagRequirements;
}

UTexture2D* URTSAbility::GetAbilityIcon() const
{
    return AbilityIcon;
}

FText URTSAbility::GetAbilityName() const
{
    return AbilityName;
}

FText URTSAbility::GetAbilityDescription() const
{
    return AbilityDescription;
}

float URTSAbility::GetDamage() const
{
    return Damage;
}

UAnimMontage* URTSAbility::GetAbilityMontage() const
{
    return AbilityMontage;
}

UNiagaraSystem* URTSAbility::GetCasterEffect() const
{
    return CasterEffect;
}

UNiagaraSystem* URTSAbility::GetTargetEffect() const
{
    return TargetEffect;
}

float URTSAbility::GetTargetEffectZOffset() const
{
    return TargetEffectZOffset;
}

float URTSAbility::GetTargetingCircleSize() const
{
    return TargetingCircleSize;
}

FKey URTSAbility::GetHotkey() const
{
    return Hotkey;
}
