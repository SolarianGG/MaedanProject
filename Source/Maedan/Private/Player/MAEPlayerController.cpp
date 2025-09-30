// Project Maedan, all right incorporated.

#include "Player/MAEPlayerController.h"

#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "MAESelectionComponent.h"
#include "NavigationSystem.h"
#include "Units/MAEUnit.h"
#include "Units/MAEUnitController.h"
#include "MAEGameMode.h"

void UMAEMoveCommand::Initialize(const FCommandData& Data)
{
	UMAECommand::Initialize(Data);

	MovePosition = Data.Location;
}

void UMAEMoveCommand::Execute(AActor* MoveActor)
{
	if (MoveActor)
	{
		auto* Controller = Cast<AMAEUnitController>(MoveActor->GetOwner());
		if (Controller)
		{
			Controller->MoveToLocation(MovePosition);
		}
	}
}

AMAEPlayerController::AMAEPlayerController()
{
	SelectionComponent = CreateDefaultSubobject<UMAESelectionComponent>("SelectionComponent");
}

void AMAEPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);

	GameMode = Cast<AMAEGameMode>(GetWorld()->GetAuthGameMode());
	check(GameMode);
	if (CameraInputMappings)
	{
		if (auto* EIS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			EIS->AddMappingContext(CameraInputMappings, 0);
		}
	}
}

void AMAEPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bIsSelecting)
	{
		OnSelectionUpdate();
	}
}

void AMAEPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (auto* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(SelectAction, ETriggerEvent::Completed, this, &ThisClass::OnSelectActor);
		EnhancedInput->BindAction(MultipleSelection, ETriggerEvent::Started, this, &ThisClass::OnSelectionStart);
		EnhancedInput->BindAction(MultipleSelection, ETriggerEvent::Completed, this, &ThisClass::OnSelectionEnd);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Started, this, &ThisClass::OnMoveCommand);
	}
}


void AMAEPlayerController::ServerIssueCommands_Implementation(const TArray<AActor*>& Actors,
	TSubclassOf<UMAECommand> Command, const FCommandData& Data)
{
	// TODO: Think about how to pass it to game mode  
}

void AMAEPlayerController::OnSelectionStart()
{
	FVector2D MousePosition;
	GetMousePosition(MousePosition.X, MousePosition.Y);

	SelectionStartPosition = MousePosition;
	SelectionComponent->StartSelectionRectangle(SelectionStartPosition);
	bIsSelecting = true;
}

void AMAEPlayerController::OnSelectionUpdate()
{
	FVector2D MousePosition;
	GetMousePosition(MousePosition.X, MousePosition.Y);

	SelectionComponent->UpdateSelectionRectangle(MousePosition);
}

void AMAEPlayerController::OnSelectionEnd()
{
	SelectionComponent->EndSelectionRectangle();
	bIsSelecting = false;

	for (const AActor* SelectedActor : SelectionComponent->CurrentlySelectedActors)
		UE_LOG(LogTemp, Warning, TEXT("Selected: %s"), *SelectedActor->GetName());
}

void AMAEPlayerController::OnSelectActor()
{
	const AActor* SelectedActor = SelectionComponent->SelectActorUnderCursor();
	if (SelectedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected: %s"), *SelectedActor->GetName());
	}
}

void AMAEPlayerController::OnMoveCommand()
{
	FVector2D MousePosition;
	GetMousePosition(MousePosition.X, MousePosition.Y);
	FVector MoveToPosition;

	// TODO: Create Helper and Use it here and in Selection Component
	if (MAEHelpers::GetWorldPositionFromMouse(this, MousePosition, MoveToPosition) && !bIsSelecting)
	{
		FCommandData Data;
		Data.Location = MoveToPosition;
		ServerIssueCommands(SelectionComponent->CurrentlySelectedActors, UMAEMoveCommand::StaticClass(), Data);
	}
}
