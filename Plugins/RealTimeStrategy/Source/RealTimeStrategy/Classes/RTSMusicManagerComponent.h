#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "RTSMusicManagerComponent.generated.h"


class UAudioComponent;
class USoundBase;


/**
 * Manages background music transitions between ambient and battle tracks.
 * Switches to battle music when owned units/buildings take damage,
 * and returns to ambient after a configurable peace timeout.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSMusicManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSMusicManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Registers an owned actor to listen for damage events. */
	void RegisterActor(AActor* Actor);

	/** Unregisters an actor from damage listening. */
	void UnregisterActor(AActor* Actor);

	/** Switches to battle music immediately. */
	UFUNCTION(BlueprintCallable, Category = "RTS|Music")
	void StartBattleMusic();

	/** Switches to ambient music immediately. */
	UFUNCTION(BlueprintCallable, Category = "RTS|Music")
	void StartAmbientMusic();

private:
	/** Ambient music track (plays by default). */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music")
	USoundBase* AmbientMusic;

	/** Battle music track (plays when units take damage). */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music")
	USoundBase* BattleMusic;

	/** Time in seconds without damage before switching back to ambient music. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music", meta = (ClampMin = 1.0))
	float PeaceTimeout;

	/** Duration of the crossfade between tracks, in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music", meta = (ClampMin = 0.0))
	float CrossfadeDuration;

	/** Audio component for the currently active track. */
	UPROPERTY()
	UAudioComponent* ActiveAudioComponent;

	/** Whether battle music is currently playing. */
	bool bIsBattleMusicPlaying;

	/** Timer handle for switching back to ambient. */
	FTimerHandle PeaceTimerHandle;

	/** Called when an owned actor takes damage. */
	UFUNCTION()
	void OnOwnedActorHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser);

	/** Called when the peace timer expires — switches back to ambient. */
	void OnPeaceTimerExpired();

	/** Plays the given sound, crossfading from the current track. */
	void PlayTrack(USoundBase* Track);
};
