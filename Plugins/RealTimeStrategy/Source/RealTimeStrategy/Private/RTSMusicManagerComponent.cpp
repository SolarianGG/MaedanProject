#include "RTSMusicManagerComponent.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Combat/RTSHealthComponent.h"
#include "RTSLog.h"


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
}

void URTSMusicManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveAudioComponent)
	{
		ActiveAudioComponent->Stop();
	}

	GetWorld()->GetTimerManager().ClearTimer(PeaceTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void URTSMusicManagerComponent::RegisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	URTSHealthComponent* HealthComp = Actor->FindComponentByClass<URTSHealthComponent>();
	if (HealthComp)
	{
		HealthComp->OnHealthChanged.AddDynamic(this, &URTSMusicManagerComponent::OnOwnedActorHealthChanged);
	}
}

void URTSMusicManagerComponent::UnregisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	URTSHealthComponent* HealthComp = Actor->FindComponentByClass<URTSHealthComponent>();
	if (HealthComp)
	{
		HealthComp->OnHealthChanged.RemoveDynamic(this, &URTSMusicManagerComponent::OnOwnedActorHealthChanged);
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
