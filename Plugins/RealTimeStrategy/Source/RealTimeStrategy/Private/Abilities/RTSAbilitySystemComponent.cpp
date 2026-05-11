#include "Abilities/RTSAbilitySystemComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "RTSLog.h"
#include "Abilities/RTSAbility.h"
#include "Combat/RTSManaComponent.h"
#include "Libraries/RTSGameplayTagLibrary.h"
#include "Orders/RTSOrderTargetData.h"
#include "Upgrades/RTSPlayerUpgradeComponent.h"


URTSAbilitySystemComponent::URTSAbilitySystemComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	InitialGameplayTags.AddTag(URTSGameplayTagLibrary::Status_Permanent_HasAbilities());
}

void URTSAbilitySystemComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update per-ability cooldown timers.
	for (int32 i = 0; i < Abilities.Num(); ++i)
	{
		if (Abilities[i].RemainingCooldown > 0)
		{
			Abilities[i].RemainingCooldown -= DeltaTime;

			if (Abilities[i].RemainingCooldown <= 0)
			{
				Abilities[i].RemainingCooldown = 0.0f;

				UE_LOG(LogRTS, Log, TEXT("Actor %s ability %d is ready again."), *GetOwner()->GetName(), i);

				// Notify listeners.
				OnAbilityCooldownReady.Broadcast(GetOwner(), i);
			}
		}
	}
}

TArray<FRTSAbilityData> URTSAbilitySystemComponent::GetAbilities() const
{
	return Abilities;
}

float URTSAbilitySystemComponent::GetRemainingCooldown(int32 AbilityIndex) const
{
	if (!Abilities.IsValidIndex(AbilityIndex))
	{
		return 0.0f;
	}

	return Abilities[AbilityIndex].RemainingCooldown;
}

bool URTSAbilitySystemComponent::CanUseAbility(int32 AbilityIndex) const
{
	if (!Abilities.IsValidIndex(AbilityIndex))
	{
		return false;
	}

	const FRTSAbilityData& AbilityData = Abilities[AbilityIndex];

	if (!AbilityData.AbilityClass)
	{
		return false;
	}

	// Check cooldown.
	if (AbilityData.RemainingCooldown > 0)
	{
		return false;
	}

	// Check mana.
	const URTSAbility* AbilityCDO = AbilityData.AbilityClass->GetDefaultObject<URTSAbility>();

	if (AbilityCDO->GetManaCost() > 0)
	{
		URTSManaComponent* ManaComponent = GetOwner()->FindComponentByClass<URTSManaComponent>();

		if (!IsValid(ManaComponent) || ManaComponent->GetCurrentMana() < AbilityCDO->GetManaCost())
		{
			return false;
		}
	}

	// Check that the required upgrade has been researched by the owning player.
	if (AbilityData.RequiredUpgrade)
	{
		AController* PC = Cast<AController>(GetOwner()->GetOwner());
		URTSPlayerUpgradeComponent* UpgradeComp =
			IsValid(PC) ? PC->FindComponentByClass<URTSPlayerUpgradeComponent>() : nullptr;

		if (!IsValid(UpgradeComp) || !UpgradeComp->HasUpgrade(AbilityData.RequiredUpgrade))
		{
			return false;
		}
	}

	// Check ability-specific conditions.
	if (!AbilityCDO->CanActivateAbility(GetOwner(), AbilityIndex))
	{
		return false;
	}

	return true;
}

void URTSAbilitySystemComponent::UseAbility(int32 AbilityIndex, AActor* TargetActor, FVector TargetLocation)
{
	AActor* Owner = GetOwner();

	// Only the server should execute abilities.
	if (!Owner->HasAuthority())
	{
		return;
	}

	if (!Abilities.IsValidIndex(AbilityIndex))
	{
		return;
	}

	if (!CanUseAbility(AbilityIndex))
	{
		return;
	}

	FRTSAbilityData& AbilityData = Abilities[AbilityIndex];
	const URTSAbility* AbilityCDO = AbilityData.AbilityClass->GetDefaultObject<URTSAbility>();

	// Build target data.
	FRTSOrderTargetData TargetData;
	TargetData.Actor = TargetActor;
	TargetData.Location = TargetLocation;

	if (IsValid(TargetActor))
	{
		TargetData.TargetTags = URTSGameplayTagLibrary::GetGameplayTags(TargetActor);
		TargetData.TargetTags.AppendTags(URTSGameplayTagLibrary::GetActorRelationshipTags(Owner, TargetActor));
	}

	// Validate target.
	if (!AbilityCDO->IsValidAbilityTarget(Owner, TargetData))
	{
		return;
	}

	// Consume mana.
	if (AbilityCDO->GetManaCost() > 0)
	{
		URTSManaComponent* ManaComponent = Owner->FindComponentByClass<URTSManaComponent>();

		if (!IsValid(ManaComponent) || !ManaComponent->ConsumeMana(AbilityCDO->GetManaCost()))
		{
			return;
		}
	}

	// Start cooldown.
	AbilityData.RemainingCooldown = AbilityCDO->GetCooldown();

	// Play animation montage.
	UAnimMontage* Montage = AbilityCDO->GetAbilityMontage();
	if (Montage)
	{
		MulticastPlayAbilityMontage(Montage);
	}

	// Spawn VFX on all clients.
	UNiagaraSystem* CasterEffect = AbilityCDO->GetCasterEffect();
	UNiagaraSystem* TargetEffect = AbilityCDO->GetTargetEffect();

	if (CasterEffect || TargetEffect)
	{
		FVector EffectTargetLocation = IsValid(TargetActor) ? TargetActor->GetActorLocation() : TargetLocation;
		EffectTargetLocation.Z += AbilityCDO->GetTargetEffectZOffset();
		MulticastSpawnAbilityEffects(CasterEffect, TargetEffect, Owner->GetActorLocation(), EffectTargetLocation, Owner->GetActorRotation());
	}

	// Activate ability.
	AbilityCDO->ActivateAbility(Owner, TargetData);

	UE_LOG(LogRTS, Log, TEXT("Actor %s used ability %d (%s)."), *Owner->GetName(), AbilityIndex, *AbilityCDO->GetAbilityName().ToString());

	// Notify listeners.
	OnAbilityUsed.Broadcast(Owner, AbilityIndex, TargetActor);

	// Replicate to clients.
	MulticastNotifyAbilityUsed(Owner, AbilityIndex, TargetActor);
}

void URTSAbilitySystemComponent::MulticastPlayAbilityMontage_Implementation(UAnimMontage* Montage)
{
	if (auto* Mesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (auto* Anim = Mesh->GetAnimInstance())
		{
			Anim->Montage_Play(Montage);
		}
	}
}

void URTSAbilitySystemComponent::MulticastNotifyAbilityUsed_Implementation(AActor* InActor, int32 InAbilityIndex, AActor* InTarget)
{
	// On clients, broadcast OnAbilityUsed.
	if (!GetOwner()->HasAuthority())
	{
		OnAbilityUsed.Broadcast(InActor, InAbilityIndex, InTarget);
	}
}

void URTSAbilitySystemComponent::MulticastSpawnAbilityEffects_Implementation(UNiagaraSystem* CasterEffect, UNiagaraSystem* TargetEffect, FVector CasterLocation, FVector TargetLocation, FRotator CasterRotation)
{
	UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	if (IsValid(CasterEffect))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, CasterEffect, CasterLocation, CasterRotation);
	}

	if (IsValid(TargetEffect))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, TargetEffect, TargetLocation);
	}
}
