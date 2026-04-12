#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "RTSMusicSwitcherComponent.generated.h"


class UAudioComponent;


/**
 * Manages background music transitions between ambient and battle tracks
 * by controlling Ambient Sound actors placed in the level.
 * Switches to battle music when owned units/buildings take damage,
 * and returns to ambient after a configurable peace timeout.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSMusicSwitcherComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSMusicSwitcherComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	/** Actor tag used to find the ambient music Ambient Sound actor in the level. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music")
	FName AmbientMusicTag;

	/** Actor tag used to find the battle music Ambient Sound actor in the level. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music")
	FName BattleMusicTag;

	/** Time in seconds without damage before switching back to ambient music. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music", meta = (ClampMin = 1.0))
	float PeaceTimeout;

	/** Duration of the crossfade between tracks, in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS|Music", meta = (ClampMin = 0.0))
	float CrossfadeDuration;

	/** Cached audio component from the ambient music Ambient Sound actor. */
	UPROPERTY()
	UAudioComponent* AmbientAudioComponent;

	/** Cached audio component from the battle music Ambient Sound actor. */
	UPROPERTY()
	UAudioComponent* BattleAudioComponent;

	/** Whether battle music is currently playing. */
	bool bIsBattleMusicPlaying;

	/** Timer handle for switching back to ambient. */
	FTimerHandle PeaceTimerHandle;

	/** Timer handle for periodic owned actor registration. */
	FTimerHandle RegistrationSyncHandle;

	/** Actors already registered for damage listening. */
	TSet<TWeakObjectPtr<AActor>> RegisteredActors;

	/** Called when an owned actor takes damage. */
	UFUNCTION()
	void OnOwnedActorHealthChanged(AActor* Actor, float OldHealth, float NewHealth, AActor* DamageCauser);

	/** Called when the peace timer expires -- switches back to ambient. */
	void OnPeaceTimerExpired();

	/** Periodically scans owned actors and registers any that are missing. */
	void SyncRegisteredActors();

	/** Finds the UAudioComponent on an Ambient Sound actor with the given tag. */
	UAudioComponent* FindMusicAudioComponent(FName Tag);
};
