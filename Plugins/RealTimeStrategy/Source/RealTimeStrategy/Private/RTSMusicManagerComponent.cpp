#include "RTSMusicManagerComponent.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Combat/RTSHealthComponent.h"
#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerController.h"
#include "RTSPlayerState.h"


URTSMusicManagerComponent::URTSMusicManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PeaceTimeout = 30.0f;
	CrossfadeDuration = 2.0f;
	bIsBattleMusicPlaying = false;
	ActiveAudioComponent = nullptr;
}

void URTSMusicManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Start ambient music.
	if (AmbientMusic)
	{
		PlayTrack(AmbientMusic);
	}

	// Periodically scan for owned actors that need registration.
	// This handles replication timing issues where actors arrive after initial discovery.
	GetWorld()->GetTimerManager().SetTimer(
		RegistrationSyncHandle,
		this,
		&URTSMusicManagerComponent::SyncRegisteredActors,
		2.0f,
		true,
		1.0f);
}

void URTSMusicManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveAudioComponent)
	{
		ActiveAudioComponent->Stop();
	}

	GetWorld()->GetTimerManager().ClearTimer(PeaceTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(RegistrationSyncHandle);

	Super::EndPlay(EndPlayReason);
}

void URTSMusicManagerComponent::RegisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// Skip if already registered.
	TWeakObjectPtr<AActor> WeakActor(Actor);
	if (RegisteredActors.Contains(WeakActor))
	{
		return;
	}

	URTSHealthComponent* HealthComp = Actor->FindComponentByClass<URTSHealthComponent>();
	if (HealthComp)
	{
		HealthComp->OnHealthChanged.AddDynamic(this, &URTSMusicManagerComponent::OnOwnedActorHealthChanged);
		RegisteredActors.Add(WeakActor);
	}
}

void URTSMusicManagerComponent::UnregisterActor(AActor* Actor)
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
		HealthComp->OnHealthChanged.RemoveDynamic(this, &URTSMusicManagerComponent::OnOwnedActorHealthChanged);
	}

	RegisteredActors.Remove(WeakActor);
}

void URTSMusicManagerComponent::SyncRegisteredActors()
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

void URTSMusicManagerComponent::OnOwnedActorHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser)
{
	// Only react to damage (health decreased), not healing.
	if (NewHealth >= OldHealth)
	{
		return;
	}

	// Reset the peace timer every time damage is taken.
	GetWorld()->GetTimerManager().ClearTimer(PeaceTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		PeaceTimerHandle,
		this,
		&URTSMusicManagerComponent::OnPeaceTimerExpired,
		PeaceTimeout,
		false);

	// Switch to battle music if not already playing.
	if (!bIsBattleMusicPlaying)
	{
		StartBattleMusic();
	}
}

void URTSMusicManagerComponent::OnPeaceTimerExpired()
{
	StartAmbientMusic();
}

void URTSMusicManagerComponent::StartBattleMusic()
{
	if (!BattleMusic)
	{
		return;
	}

	bIsBattleMusicPlaying = true;
	PlayTrack(BattleMusic);

	UE_LOG(LogRTS, Log, TEXT("Music: switching to battle track."));
}

void URTSMusicManagerComponent::StartAmbientMusic()
{
	if (!AmbientMusic)
	{
		return;
	}

	bIsBattleMusicPlaying = false;
	PlayTrack(AmbientMusic);

	UE_LOG(LogRTS, Log, TEXT("Music: switching to ambient track."));
}

void URTSMusicManagerComponent::PlayTrack(USoundBase* Track)
{
	if (!Track)
	{
		return;
	}

	// Fade out the current track.
	if (ActiveAudioComponent && ActiveAudioComponent->IsPlaying())
	{
		ActiveAudioComponent->FadeOut(CrossfadeDuration, 0.0f);
	}

	// Create a new audio component for the new track.
	ActiveAudioComponent = UGameplayStatics::SpawnSound2D(GetWorld(), Track, 1.0f, 1.0f, 0.0f);
	if (ActiveAudioComponent)
	{
		ActiveAudioComponent->bAutoDestroy = false;
		ActiveAudioComponent->FadeIn(CrossfadeDuration);
	}
}
