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
	}
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

	for (const auto* SelectedActor : SelectionComponent->CurrentlySelectedActors)
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected: %s"), *SelectedActor->GetName());
	}
}

void AMAEPlayerController::OnSelectActor()
{
	AActor* SelectedActor = SelectionComponent->SelectActorUnderCursor();
	if (!SelectedActor) return;

	UE_LOG(LogTemp, Warning, TEXT("Selected: %s"), *SelectedActor->GetName());

	if (auto* EIS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		EIS->ClearAllMappings();
		if (CameraInputMappings)
			EIS->AddMappingContext(CameraInputMappings, 0);
	}

	if (auto* InputComp = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (auto* Selectable = Cast<IMAESelectableInterface>(SelectedActor))
		{
			if (auto* Context = Selectable->Execute_GetInputMapping(SelectedActor))
			{
				if (auto* EIS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
				{
					EIS->AddMappingContext(Context, 1);
				}
			}

			Selectable->Execute_SetupSelectionInput(SelectedActor, InputComp, this);
			Selectable->Execute_OnSelected(SelectedActor, this);
		}
	}
}
