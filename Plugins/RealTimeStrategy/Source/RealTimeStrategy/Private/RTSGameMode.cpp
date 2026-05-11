#include "RTSGameMode.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

#include "RTSGameState.h"
#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerAIController.h"
#include "RTSPlayerAdvantageComponent.h"
#include "RTSPlayerController.h"
#include "RTSPlayerStart.h"
#include "RTSPlayerState.h"
#include "RTSTeamInfo.h"
#include "Construction/RTSConstructionSiteComponent.h"
#include "Vision/RTSVisionInfo.h"


ARTSGameMode::ARTSGameMode(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	// Set reasonable default values.
    TeamClass = ARTSTeamInfo::StaticClass();
	NumTeams = 2;
	bGameHasEnded = false;
}

void ARTSGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Parse options.
    FString NumAIPlayersString = UGameplayStatics::ParseOption(OptionsString, TEXT("NumAIPlayers"));

    if (!NumAIPlayersString.IsEmpty())
    {
        NumAIPlayers = FCString::Atoi(*NumAIPlayersString);
    }

    UE_LOG(LogRTS, Log, TEXT("NumAIPlayers = %i"), NumAIPlayers);

    // Spawn AI players.
    for (int32 Index = 0; Index < NumAIPlayers; ++Index)
    {
        ARTSPlayerAIController* NewAI = StartAIPlayer();

        if (NewAI != nullptr)
        {
            NewAI->PlayerState->SetPlayerName(FString::Printf(TEXT("AI Player %i"), Index + 1));
        }
    }
}

void ARTSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	AGameModeBase::InitGame(MapName, Options, ErrorMessage);

	// Set up teams.
    if (TeamClass == nullptr)
    {
        TeamClass = ARTSTeamInfo::StaticClass();
    }

	for (uint8 TeamIndex = 0; TeamIndex < NumTeams; ++TeamIndex)
	{
		// Add team.
		ARTSTeamInfo* NewTeam = GetWorld()->SpawnActor<ARTSTeamInfo>(TeamClass);
		NewTeam->SetTeamIndex(TeamIndex);

        if (Teams.Num() <= TeamIndex)
        {
            Teams.SetNum(TeamIndex + 1);
        }

        Teams[TeamIndex] = NewTeam;

		// Setup vision.
		ARTSVisionInfo* TeamVision = GetWorld()->SpawnActor<ARTSVisionInfo>();
		TeamVision->SetTeamIndex(TeamIndex);

        UE_LOG(LogRTS, Log, TEXT("Set up team %s (team index %i)."), *NewTeam->GetName(), TeamIndex);
	}
}

ARTSPlayerStart* ARTSGameMode::FindRTSPlayerStart(AController* Player)
{
	// Choose a player start.
	TArray<ARTSPlayerStart*> UnOccupiedStartPoints;
	TArray<ARTSPlayerStart*> OccupiedStartPoints;

	for (TActorIterator<ARTSPlayerStart> It(GetWorld()); It; ++It)
	{
		ARTSPlayerStart* PlayerStart = *It;

		if (PlayerStart->GetPlayer() == nullptr)
		{
			UnOccupiedStartPoints.Add(PlayerStart);
		}
		else
		{
			OccupiedStartPoints.Add(PlayerStart);
		}
	}

	if (UnOccupiedStartPoints.Num() > 0)
	{
		return UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
	}
	else if (OccupiedStartPoints.Num() > 0)
	{
		return OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
	}

	return nullptr;
}

void ARTSGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	ARTSPlayerStart* StartSpot = FindRTSPlayerStart(NewPlayer);
	RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

void ARTSGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	// Spawn default pawns (most likely, this will be the camera pawn in our case).
	AGameModeBase::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);

	// Verify parameters.
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	if (!StartSpot)
	{
		return;
	}

    ARTSGameState* RTSGameState = GetGameState<ARTSGameState>();

    if (!IsValid(RTSGameState))
    {
        return;
    }

	// Occupy start spot.
	ARTSPlayerStart* PlayerStart = Cast<ARTSPlayerStart>(StartSpot);

	if (PlayerStart)
	{
		UE_LOG(LogRTS, Log, TEXT("Start spot %s is now occupied by player %s."), *PlayerStart->GetName(), *NewPlayer->GetName());
		PlayerStart->SetPlayer(NewPlayer);
	}

	// Set team.
	if (PlayerStart->GetTeamIndex() >= Teams.Num())
	{
		UE_LOG(LogRTS, Warning, TEXT("Player start team index is %d, but game only has %d teams."), PlayerStart->GetTeamIndex(), Teams.Num());
	}
	else
	{
		Teams[PlayerStart->GetTeamIndex()]->AddToTeam(NewPlayer);
	}

	// Build spawn transform.
	// Don't allow initial actors to be spawned with any pitch or roll.
	FRotator SpawnRotation(ForceInit);
	SpawnRotation.Yaw = StartSpot->GetActorRotation().Yaw;

	// Build spawn info.
	for (int32 Index = 0; Index < InitialActors.Num(); ++Index)
	{
        TSubclassOf<AActor> ActorClass = InitialActors[Index];

		// Spawn actor.
        FVector SpawnLocation = StartSpot->GetActorLocation();

        if (Index < InitialActorLocations.Num())
        {
            SpawnLocation += InitialActorLocations[Index];
        }
        
        FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);
		AActor* SpawnedActor = SpawnActorForPlayer(ActorClass, NewPlayer, SpawnTransform);

        // Finish construction of initial buildings immediately.
        if (SpawnedActor != nullptr)
        {
            URTSConstructionSiteComponent* ConstructionSiteComponent = SpawnedActor->FindComponentByClass<URTSConstructionSiteComponent>();

            if (ConstructionSiteComponent != nullptr)
            {
                ConstructionSiteComponent->FinishConstruction();
            }
        }
	}

    // Transfer ownership of pre-placed units.
    ARTSPlayerState* PlayerState = Cast<ARTSPlayerState>(NewPlayer->PlayerState);

    if (IsValid(PlayerState))
    {
        uint8 PlayerIndex = GetAvailablePlayerIndex();
        PlayerState->SetPlayerIndex(PlayerIndex);

        for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
        {
            AActor* Actor = *ActorItr;

            // Check owner.
            URTSOwnerComponent* OwnerComponent = Actor->FindComponentByClass<URTSOwnerComponent>();

            if (IsValid(OwnerComponent) && OwnerComponent->GetInitialOwnerPlayerIndex() == PlayerIndex)
            {
                TransferOwnership(Actor, NewPlayer);
            }
        }
    }
}

ARTSPlayerAIController* ARTSGameMode::StartAIPlayer()
{
    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Instigator = GetInstigator();
    SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save player controllers into a map
    SpawnInfo.bDeferConstruction = true;
    ARTSPlayerAIController* NewAI = GetWorld()->SpawnActor<ARTSPlayerAIController>(PlayerAIControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
    if (NewAI)
    {
        UGameplayStatics::FinishSpawningActor(NewAI, FTransform(FRotator::ZeroRotator, FVector::ZeroVector));

        UE_LOG(LogRTS, Log, TEXT("Spawned AI player %s."), *NewAI->GetName());
    }
    else
    {
        UE_LOG(LogRTS, Error, TEXT("Failed to spawn AI player."));
        return nullptr;
    }

    RestartPlayer(NewAI);
    return NewAI;
}

AActor* ARTSGameMode::SpawnActorForPlayer(TSubclassOf<AActor> ActorClass, AController* ActorOwner, const FTransform& SpawnTransform)
{
	// Spawn actor.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass->GetDefaultObject()->GetClass(), SpawnTransform, SpawnParams);

	// Set owning player.
	if (SpawnedActor && ActorOwner)
	{
		UE_LOG(LogRTS, Log, TEXT("Spawned %s for player %s at %s."), *SpawnedActor->GetName(), *ActorOwner->GetName(), *SpawnTransform.GetLocation().ToString());

		// Set owning player.
		TransferOwnership(SpawnedActor, ActorOwner);
	}

	return SpawnedActor;
}

void ARTSGameMode::TransferOwnership(AActor* Actor, AController* NewOwner)
{
    if (!Actor || !NewOwner)
    {
        return;
    }

    // Set owning player.
    Actor->SetOwner(NewOwner);

    URTSOwnerComponent* OwnerComponent = Actor->FindComponentByClass<URTSOwnerComponent>();

    if (OwnerComponent)
    {
        OwnerComponent->SetPlayerOwner(NewOwner);
    }

    UE_LOG(LogRTS, Log, TEXT("Player %s is now owning %s."), *NewOwner->GetName(), *Actor->GetName());

    // Check for god mode.
    URTSPlayerAdvantageComponent* PlayerAdvantageComponent = NewOwner->FindComponentByClass<URTSPlayerAdvantageComponent>();

    if (PlayerAdvantageComponent)
    {
        APawn* Pawn = Cast<APawn>(Actor);

        if (Pawn)
        {
            Pawn->SetCanBeDamaged(!PlayerAdvantageComponent->IsGodModeEnabled());
        }
    }   
}

TArray<ARTSTeamInfo*> ARTSGameMode::GetTeams() const
{
    return Teams;
}

void ARTSGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);

    if (Cast<ARTSPlayerController>(Exiting) != nullptr)
    {
        UE_LOG(LogRTS, Log, TEXT("Player %s disconnected and is now defeated."), *Exiting->GetName());
        NotifyOnPlayerDefeated(Exiting);
    }
}

void ARTSGameMode::NotifyOnActorKilled(AActor* Actor, AController* ActorOwner)
{
    ARTSPlayerController* OwningPlayer = Cast<ARTSPlayerController>(ActorOwner);

    if (OwningPlayer == nullptr)
    {
        ARTSPlayerAIController* OwningAIPlayer = Cast<ARTSPlayerAIController>(ActorOwner);

        if (OwningAIPlayer == nullptr)
        {
            return;
        }
    }

    // Defeat is triggered only when a building (actor with URTSConstructionSiteComponent) dies.
    if (!IsValid(Actor->FindComponentByClass<URTSConstructionSiteComponent>()))
    {
        return;
    }

    // Check if the player still has at least one building alive (excluding the dying actor).
    for (AActor* OwnedActor : ActorOwner->Children)
    {
        if (OwnedActor == Actor)
        {
            continue;
        }

        if (IsValid(OwnedActor->FindComponentByClass<URTSConstructionSiteComponent>()))
        {
            return;
        }
    }

    UE_LOG(LogRTS, Log, TEXT("Player %s has no buildings left and has been defeated."), *ActorOwner->GetName());

    NotifyOnPlayerDefeated(ActorOwner);
}

void ARTSGameMode::NotifyOnPlayerDefeated(AController* Player)
{
    // Only process recognized RTS player types.
    if (Cast<ARTSPlayerController>(Player) == nullptr &&
        Cast<ARTSPlayerAIController>(Player) == nullptr)
    {
        return;
    }

    // Guard against duplicate defeat notifications.
    if (DefeatedPlayers.Contains(Player))
    {
        return;
    }

    // Do not process after a winner has already been declared.
    if (bGameHasEnded)
    {
        return;
    }

    DefeatedPlayers.Add(Player);
    UE_LOG(LogRTS, Log, TEXT("Player %s has been defeated."), *Player->GetName());

    // Fire the existing Blueprint event.
    ReceiveOnPlayerDefeated(Player);

    // Notify the defeated player's controller (triggers ClientGameHasEnded RPC -> Blueprint UI).
    Player->GameHasEnded(nullptr, false);

    // Do not continue during engine teardown.
    UWorld* World = GetWorld();
    if (!IsValid(World) || World->bIsTearingDown)
    {
        return;
    }

    // Find all remaining non-defeated RTS players.
    AController* LastSurvivor = nullptr;
    int32 SurvivorCount = 0;

    for (TActorIterator<AController> It(World); It; ++It)
    {
        AController* Controller = *It;
        if (!IsValid(Controller)) continue;
        if (Cast<ARTSPlayerController>(Controller) == nullptr &&
            Cast<ARTSPlayerAIController>(Controller) == nullptr) continue;
        if (DefeatedPlayers.Contains(Controller)) continue;

        LastSurvivor = Controller;
        ++SurvivorCount;
    }

    // Last player standing wins. Zero survivors = draw, no winner is declared.
    if (SurvivorCount == 1 && LastSurvivor != nullptr)
    {
        bGameHasEnded = true;
        UE_LOG(LogRTS, Log, TEXT("Player %s is the last player standing and wins!"), *LastSurvivor->GetName());
        LastSurvivor->GameHasEnded(nullptr, true);
    }
}

uint8 ARTSGameMode::GetAvailablePlayerIndex()
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return ARTSPlayerState::PLAYER_INDEX_NONE;
    }

    uint8 PlayerIndex = 0;
    bool bPlayerIndexInUse = false;

    do
    {
        bPlayerIndexInUse = false;

        for (TActorIterator<ARTSPlayerState> PlayerIt(World); PlayerIt; ++PlayerIt)
        {
            ARTSPlayerState* PlayerState = *PlayerIt;

            if (PlayerState->GetPlayerIndex() == PlayerIndex)
            {
                bPlayerIndexInUse = true;
                ++PlayerIndex;
                break;
            }
        }
    } while (bPlayerIndexInUse && PlayerIndex < ARTSPlayerState::PLAYER_INDEX_NONE);

    return PlayerIndex;
}
