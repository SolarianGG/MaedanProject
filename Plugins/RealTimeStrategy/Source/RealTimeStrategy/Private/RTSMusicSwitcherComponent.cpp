#include "RTSMusicSwitcherComponent.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/AmbientSound.h"
#include "TimerManager.h"

#include "Combat/RTSAttackComponent.h"
#include "Combat/RTSHealthComponent.h"
#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerController.h"
#include "RTSPlayerState.h"


URTSMusicSwitcherComponent::URTSMusicSwitcherComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AmbientMusicTag = FName(TEXT("MusicAmbient"));
	BattleMusicTag = FName(TEXT("MusicBattle"));
	PeaceTimeout = 30.0f;
	CrossfadeDuration = 2.0f;
	bIsBattleMusicPlaying = false;
	AmbientAudioComponent = nullptr;
	BattleAudioComponent = nullptr;
}

void URTSMusicSwitcherComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find the Ambient Sound actors in the level by tag.
	AmbientAudioComponent = FindMusicAudioComponent(AmbientMusicTag);
	BattleAudioComponent = FindMusicAudioComponent(BattleMusicTag);

	// Start ambient music.
	if (AmbientAudioComponent)
	{
		AmbientAudioComponent->FadeIn(CrossfadeDuration);
	}

	// Periodically scan for owned actors that need registration.
	GetWorld()->GetTimerManager().SetTimer(
		RegistrationSyncHandle,
		this,
		&URTSMusicSwitcherComponent::SyncRegisteredActors,
		2.0f,
		true,
		1.0f);
}

void URTSMusicSwitcherComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AmbientAudioComponent && AmbientAudioComponent->IsPlaying())
	{
		AmbientAudioComponent->FadeOut(CrossfadeDuration, 0.0f);
	}

	if (BattleAudioComponent && BattleAudioComponent->IsPlaying())
	{
		BattleAudioComponent->FadeOut(CrossfadeDuration, 0.0f);
	}

	GetWorld()->GetTimerManager().ClearTimer(PeaceTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(RegistrationSyncHandle);

	Super::EndPlay(EndPlayReason);
}

void URTSMusicSwitcherComponent::RegisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	TWeakObjectPtr<AActor> WeakActor(Actor);
	if (RegisteredActors.Contains(WeakActor))
	{
		return;
	}

	bool bShouldRegister = false;

	URTSHealthComponent* HealthComp = Actor->FindComponentByClass<URTSHealthComponent>();
	if (HealthComp)
	{
		HealthComp->OnHealthChanged.AddDynamic(this, &URTSMusicSwitcherComponent::OnOwnedActorHealthChanged);
		bShouldRegister = true;
	}

	URTSAttackComponent* AttackComp = Actor->FindComponentByClass<URTSAttackComponent>();
	if (AttackComp)
	{
		AttackComp->OnAttackUsed.AddDynamic(this, &URTSMusicSwitcherComponent::OnOwnedActorAttackUsed);
		bShouldRegister = true;
	}

	if (bShouldRegister)
	{
		RegisteredActors.Add(WeakActor);
	}
}

void URTSMusicSwitcherComponent::UnregisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	TWeakObjectPtr<AActor> WeakActor(Actor);
	if (!RegisteredActors.Contains(WeakActor))
	{
		return;
	}

	URTSHealthComponent* HealthComp = Actor->FindComponentByClass<URTSHealthComponent>();
	if (HealthComp)
	{
		HealthComp->OnHealthChanged.RemoveDynamic(this, &URTSMusicSwitcherComponent::OnOwnedActorHealthChanged);
	}

	URTSAttackComponent* AttackComp = Actor->FindComponentByClass<URTSAttackComponent>();
	if (AttackComp)
	{
		AttackComp->OnAttackUsed.RemoveDynamic(this, &URTSMusicSwitcherComponent::OnOwnedActorAttackUsed);
	}

	RegisteredActors.Remove(WeakActor);
}

void URTSMusicSwitcherComponent::SyncRegisteredActors()
{
	// Clean up stale entries.
	for (auto It = RegisteredActors.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	// Find the owning player controller and its player state.
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	ARTSPlayerState* PS = PC->GetPlayerState();
	if (!PS)
	{
		return;
	}

	// Register any owned actors that aren't registered yet.
	for (AActor* OwnedActor : PS->GetOwnActors())
	{
		if (IsValid(OwnedActor))
		{
			RegisterActor(OwnedActor);
		}
	}
}

void URTSMusicSwitcherComponent::OnOwnedActorHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser)
{
	// Only react to damage (health decreased), not healing.
	if (NewHealth >= OldHealth)
	{
		return;
	}

	TriggerBattleMusic();
}

void URTSMusicSwitcherComponent::OnOwnedActorAttackUsed(AActor* Actor, const FRTSAttackData& Attack, AActor* Target, ARTSProjectile* Projectile)
{
	TriggerBattleMusic();
}

void URTSMusicSwitcherComponent::TriggerBattleMusic()
{
	// Reset the peace timer.
	GetWorld()->GetTimerManager().ClearTimer(PeaceTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		PeaceTimerHandle,
		this,
		&URTSMusicSwitcherComponent::OnPeaceTimerExpired,
		PeaceTimeout,
		false);

	// Switch to battle music if not already playing.
	if (!bIsBattleMusicPlaying)
	{
		StartBattleMusic();
	}
}

void URTSMusicSwitcherComponent::OnPeaceTimerExpired()
{
	StartAmbientMusic();
}

void URTSMusicSwitcherComponent::StartBattleMusic()
{
	if (!BattleAudioComponent)
	{
		return;
	}

	if (AmbientAudioComponent && AmbientAudioComponent->IsPlaying())
	{
		AmbientAudioComponent->FadeOut(CrossfadeDuration, 0.0f);
	}

	BattleAudioComponent->FadeIn(CrossfadeDuration);
	bIsBattleMusicPlaying = true;

	UE_LOG(LogRTS, Log, TEXT("Music: switching to battle track."));
}

void URTSMusicSwitcherComponent::StartAmbientMusic()
{
	if (!AmbientAudioComponent)
	{
		return;
	}

	if (BattleAudioComponent && BattleAudioComponent->IsPlaying())
	{
		BattleAudioComponent->FadeOut(CrossfadeDuration, 0.0f);
	}

	AmbientAudioComponent->FadeIn(CrossfadeDuration);
	bIsBattleMusicPlaying = false;

	UE_LOG(LogRTS, Log, TEXT("Music: switching to ambient track."));
}

UAudioComponent* URTSMusicSwitcherComponent::FindMusicAudioComponent(FName Tag)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, FoundActors);

	if (FoundActors.Num() == 0)
	{
		UE_LOG(LogRTS, Warning, TEXT("Music: no Ambient Sound actor found with tag '%s'."), *Tag.ToString());
		return nullptr;
	}

	if (FoundActors.Num() > 1)
	{
		UE_LOG(LogRTS, Warning, TEXT("Music: multiple Ambient Sound actors found with tag '%s', using the first one."), *Tag.ToString());
	}

	AAmbientSound* AmbientSound = Cast<AAmbientSound>(FoundActors[0]);
	if (!AmbientSound)
	{
		UE_LOG(LogRTS, Warning, TEXT("Music: actor with tag '%s' is not an AAmbientSound."), *Tag.ToString());
		return nullptr;
	}

	return AmbientSound->GetAudioComponent();
}
