#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameModeBase.h"
#include "Templates/SubclassOf.h"

#include "RTSGameMode.generated.h"


class AController;

class ARTSPlayerAIController;
class ARTSPlayerController;
class ARTSPlayerStart;
class ARTSTeamInfo;


/**
* Game mode with RTS features, such as spawning initial units for each player.
*/
UCLASS()
class REALTIMESTRATEGY_API ARTSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARTSGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


    virtual void BeginPlay() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	ARTSPlayerStart* FindRTSPlayerStart(AController* Player);

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;

    virtual ARTSPlayerAIController* StartAIPlayer();

	/** Spawns an actor of the specified type and transfers ownership to the specified player. */
	virtual AActor* SpawnActorForPlayer(TSubclassOf<AActor> ActorClass, AController* ActorOwner, const FTransform& SpawnTransform);

    /** Sets the specified player as the owner of the passed actor. */
    UFUNCTION(BlueprintCallable)
    void TransferOwnership(AActor* Actor, AController* NewOwner);

    /** Gets the teams of the current match. */
    TArray<ARTSTeamInfo*> GetTeams() const;

	/** Event when an actor has been killed. */
	virtual void NotifyOnActorKilled(AActor* Actor, AController* ActorOwner);

	/** Event when a player has been defeated. */
	virtual void NotifyOnPlayerDefeated(AController* Player);

	/** Defeat a player when they disconnect. */
	virtual void Logout(AController* Exiting) override;

	/** Event when a player has been defeated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RTS", meta = (DisplayName = "OnPlayerDefeated"))
	void ReceiveOnPlayerDefeated(AController* Player);

protected:
	/** Sets the number of teams to create. Must be called before InitGame spawns the teams. */
	void SetNumTeams(uint8 InNumTeams);

private:
	/** Controllers that have already been defeated. Guards against duplicate defeat processing. */
	UPROPERTY(Transient)
	TSet<AController*> DefeatedPlayers;

	/** True once a winner has been declared. Prevents multiple winners if two players' last buildings die simultaneously. */
	bool bGameHasEnded;

    /** Actors to spawn for each player in the game. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TArray<TSubclassOf<AActor>> InitialActors;

    /** Relative locations of the actors to spawn for each player in the game, relative to their respective start spot. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS")
    TArray<FVector> InitialActorLocations;

    /** Class of TeamInfo to spawn. */
    UPROPERTY(EditDefaultsOnly, Category = "Team")
    TSubclassOf<ARTSTeamInfo> TeamClass;

    /** Number of teams to create. */
    UPROPERTY(EditDefaultsOnly, Category = "Team", meta = (ClampMin = 0))
    uint8 NumTeams;

    /** AIController class to spawn for AI players. */
    UPROPERTY(EditDefaultsOnly, Category = AI)
    TSubclassOf<ARTSPlayerAIController> PlayerAIControllerClass;

    /** Number of AI players to spawn. */
    UPROPERTY(EditDefaultsOnly, Category = AI)
    int32 NumAIPlayers;

    /** Teams of the current match. */
    UPROPERTY()
    TArray<ARTSTeamInfo*> Teams;

    /** Gets the first player index that isn't assigned to any player. */
    uint8 GetAvailablePlayerIndex();
};
