#include "RTSPlayerController.h"

#include "EngineUtils.h"
#include "Landscape.h"
#include "Camera/CameraComponent.h"
#include "Components/BrushComponent.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

#include "UI/RTSHUD.h"
#include "RTSCameraBoundsVolume.h"
#include "RTSPawnAIController.h"
#include "RTSGameMode.h"
#include "RTSGameState.h"
#include "RTSLog.h"
#include "RTSNameComponent.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerAdvantageComponent.h"
#include "RTSPlayerState.h"
#include "RTSSelectableComponent.h"
#include "RTSTeamInfo.h"
#include "Combat/RTSAttackComponent.h"
#include "Construction/RTSBuilderComponent.h"
#include "Construction/RTSBuildingCursor.h"
#include "Construction/RTSConstructionSiteComponent.h"
#include "Economy/RTSGathererComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UI/RTSRallyPointIndicator.h"
#include "Economy/RTSPlayerResourcesComponent.h"
#include "Economy/RTSResourceSourceComponent.h"
#include "Libraries/RTSCollisionLibrary.h"
#include "Libraries/RTSFormationHelper.h"
#include "Libraries/RTSConstructionLibrary.h"
#include "Libraries/RTSGameplayLibrary.h"
#include "Libraries/RTSGameplayTagLibrary.h"
#include "Libraries/RTSOrderLibrary.h"
#include "Orders/RTSAttackOrder.h"
#include "Orders/RTSBeginConstructionOrder.h"
#include "Orders/RTSContinueConstructionOrder.h"
#include "Orders/RTSGatherOrder.h"
#include "Orders/RTSMoveOrder.h"
#include "Orders/RTSReturnResourcesOrder.h"
#include "Orders/RTSSetRallyPointToActorOrder.h"
#include "Orders/RTSSetRallyPointToLocationOrder.h"
#include "Orders/RTSStopOrder.h"
#include "Production/RTSProductionComponent.h"
#include "Production/RTSProductionCostComponent.h"
#include "UI/RTSMinimapVolume.h"
#include "Vision/RTSVisibleComponent.h"
#include "TimerManager.h"
#include "Vision/RTSFogOfWarActor.h"
#include "Vision/RTSVisionInfo.h"
#include "Vision/RTSVisionManager.h"
#include "Vision/RTSVisionVolume.h"


ARTSPlayerController::ARTSPlayerController(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	PlayerAdvantageComponent = CreateDefaultSubobject<URTSPlayerAdvantageComponent>(TEXT("Player Advantage"));
	PlayerResourcesComponent = CreateDefaultSubobject<URTSPlayerResourcesComponent>(TEXT("Player Resources"));

	// Set reasonable default values.
	CameraSpeed = 1000.0f;
	CameraZoomSpeed = 4000.0f;

	MinCameraDistance = 500.0f;
	MaxCameraDistance = 2500.0f;

	CameraScrollThreshold = 50.0f;
	EdgeScrollSpeedMultiplier = 1.5f;

	CameraPitchSpeed = 50.0f;
	MinCameraPitch = -80.0f;
	MaxCameraPitch = -20.0f;
	CameraPitchAxisValue = 0.0f;
	bRotatingCamera = false;

	DoubleGroupSelectionTime = 0.2f;

	FormationUnitSpacing = 150.0f;

	DefaultOrders.Add(URTSAttackOrder::StaticClass());
	DefaultOrders.Add(URTSGatherOrder::StaticClass());
	DefaultOrders.Add(URTSContinueConstructionOrder::StaticClass());
	DefaultOrders.Add(URTSReturnResourcesOrder::StaticClass());
	DefaultOrders.Add(URTSMoveOrder::StaticClass());
	DefaultOrders.Add(URTSSetRallyPointToActorOrder::StaticClass());
	DefaultOrders.Add(URTSSetRallyPointToLocationOrder::StaticClass());

	DefaultOrderIgnoreTargetClasses.Add(ARTSCameraBoundsVolume::StaticClass());
	DefaultOrderIgnoreTargetClasses.Add(ARTSVisionVolume::StaticClass());
	DefaultOrderIgnoreTargetClasses.Add(ARTSMinimapVolume::StaticClass());
}

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Allow immediate updates for interested listeners.
	for (int32 Index = 0; Index < PlayerResourcesComponent->GetResourceTypes().Num(); ++Index)
	{
		PlayerResourcesComponent->OnResourcesChanged.Broadcast(
			PlayerResourcesComponent->GetResourceTypes()[Index],
			0.0f,
			PlayerResourcesComponent->GetResources(PlayerResourcesComponent->GetResourceTypes()[Index]),
			true);
	}
}

void ARTSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enable mouse input.
	APlayerController::bShowMouseCursor = true;
	APlayerController::bEnableClickEvents = true;
	APlayerController::bEnableMouseOverEvents = true;

	// Bind actions.
	InputComponent->BindAction(TEXT("Select"), IE_Pressed, this, &ARTSPlayerController::StartSelectActors);
	InputComponent->BindAction(TEXT("Select"), IE_Released, this, &ARTSPlayerController::FinishSelectActors);
	InputComponent->BindAction(TEXT("AddSelection"), IE_Pressed, this, &ARTSPlayerController::StartAddSelection);
	InputComponent->BindAction(TEXT("AddSelection"), IE_Released, this, &ARTSPlayerController::StopAddSelection);
	InputComponent->BindAction(TEXT("ToggleSelection"), IE_Pressed, this, &ARTSPlayerController::StartToggleSelection);
	InputComponent->BindAction(TEXT("ToggleSelection"), IE_Released, this, &ARTSPlayerController::StopToggleSelection);

	InputComponent->BindAction(TEXT("SelectNextSubgroup"), IE_Pressed, this, &ARTSPlayerController::SelectNextSubgroup);
	InputComponent->BindAction(TEXT("SelectPreviousSubgroup"), IE_Pressed, this,
	                           &ARTSPlayerController::SelectPreviousSubgroup);

	InputComponent->BindAction(TEXT("IssueOrder"), IE_Pressed, this,
	                           &ARTSPlayerController::StartContinuousOrder);
	InputComponent->BindAction(TEXT("IssueOrder"), IE_Released, this,
	                           &ARTSPlayerController::StopContinuousOrder);
	InputComponent->BindAction(TEXT("IssueStopOrder"), IE_Released, this, &ARTSPlayerController::IssueStopOrder);

	InputComponent->BindAction(TEXT("SaveControlGroup0"), IE_Released, this, &ARTSPlayerController::SaveControlGroup0);
	InputComponent->BindAction(TEXT("SaveControlGroup1"), IE_Released, this, &ARTSPlayerController::SaveControlGroup1);
	InputComponent->BindAction(TEXT("SaveControlGroup2"), IE_Released, this, &ARTSPlayerController::SaveControlGroup2);
	InputComponent->BindAction(TEXT("SaveControlGroup3"), IE_Released, this, &ARTSPlayerController::SaveControlGroup3);
	InputComponent->BindAction(TEXT("SaveControlGroup4"), IE_Released, this, &ARTSPlayerController::SaveControlGroup4);
	InputComponent->BindAction(TEXT("SaveControlGroup5"), IE_Released, this, &ARTSPlayerController::SaveControlGroup5);
	InputComponent->BindAction(TEXT("SaveControlGroup6"), IE_Released, this, &ARTSPlayerController::SaveControlGroup6);
	InputComponent->BindAction(TEXT("SaveControlGroup7"), IE_Released, this, &ARTSPlayerController::SaveControlGroup7);
	InputComponent->BindAction(TEXT("SaveControlGroup8"), IE_Released, this, &ARTSPlayerController::SaveControlGroup8);
	InputComponent->BindAction(TEXT("SaveControlGroup9"), IE_Released, this, &ARTSPlayerController::SaveControlGroup9);

	InputComponent->BindAction(TEXT("LoadControlGroup0"), IE_Released, this, &ARTSPlayerController::LoadControlGroup0);
	InputComponent->BindAction(TEXT("LoadControlGroup1"), IE_Released, this, &ARTSPlayerController::LoadControlGroup1);
	InputComponent->BindAction(TEXT("LoadControlGroup2"), IE_Released, this, &ARTSPlayerController::LoadControlGroup2);
	InputComponent->BindAction(TEXT("LoadControlGroup3"), IE_Released, this, &ARTSPlayerController::LoadControlGroup3);
	InputComponent->BindAction(TEXT("LoadControlGroup4"), IE_Released, this, &ARTSPlayerController::LoadControlGroup4);
	InputComponent->BindAction(TEXT("LoadControlGroup5"), IE_Released, this, &ARTSPlayerController::LoadControlGroup5);
	InputComponent->BindAction(TEXT("LoadControlGroup6"), IE_Released, this, &ARTSPlayerController::LoadControlGroup6);
	InputComponent->BindAction(TEXT("LoadControlGroup7"), IE_Released, this, &ARTSPlayerController::LoadControlGroup7);
	InputComponent->BindAction(TEXT("LoadControlGroup8"), IE_Released, this, &ARTSPlayerController::LoadControlGroup8);
	InputComponent->BindAction(TEXT("LoadControlGroup9"), IE_Released, this, &ARTSPlayerController::LoadControlGroup9);

	InputComponent->BindAxis(TEXT("MoveCameraLeftRight"), this, &ARTSPlayerController::MoveCameraLeftRight);
	InputComponent->BindAxis(TEXT("MoveCameraUpDown"), this, &ARTSPlayerController::MoveCameraUpDown);
	InputComponent->BindAxis(TEXT("ZoomCamera"), this, &ARTSPlayerController::ZoomCamera);

	InputComponent->BindAction(TEXT("RotateCamera"), IE_Pressed, this, &ARTSPlayerController::StartRotateCamera);
	InputComponent->BindAction(TEXT("RotateCamera"), IE_Released, this, &ARTSPlayerController::StopRotateCamera);
	InputComponent->BindAxis(TEXT("RotateCameraPitch"), this, &ARTSPlayerController::RotateCameraPitch);

	InputComponent->BindAction(TEXT("SaveCameraLocation0"), IE_Pressed, this,
	                           &ARTSPlayerController::SaveCameraLocationWithIndex<0>);
	InputComponent->BindAction(TEXT("SaveCameraLocation1"), IE_Pressed, this,
	                           &ARTSPlayerController::SaveCameraLocationWithIndex<1>);
	InputComponent->BindAction(TEXT("SaveCameraLocation2"), IE_Pressed, this,
	                           &ARTSPlayerController::SaveCameraLocationWithIndex<2>);
	InputComponent->BindAction(TEXT("SaveCameraLocation3"), IE_Pressed, this,
	                           &ARTSPlayerController::SaveCameraLocationWithIndex<3>);
	InputComponent->BindAction(TEXT("SaveCameraLocation4"), IE_Pressed, this,
	                           &ARTSPlayerController::SaveCameraLocationWithIndex<4>);

	InputComponent->BindAction(TEXT("LoadCameraLocation0"), IE_Released, this,
	                           &ARTSPlayerController::LoadCameraLocationWithIndex<0>);
	InputComponent->BindAction(TEXT("LoadCameraLocation1"), IE_Released, this,
	                           &ARTSPlayerController::LoadCameraLocationWithIndex<1>);
	InputComponent->BindAction(TEXT("LoadCameraLocation2"), IE_Released, this,
	                           &ARTSPlayerController::LoadCameraLocationWithIndex<2>);
	InputComponent->BindAction(TEXT("LoadCameraLocation3"), IE_Released, this,
	                           &ARTSPlayerController::LoadCameraLocationWithIndex<3>);
	InputComponent->BindAction(TEXT("LoadCameraLocation4"), IE_Released, this,
	                           &ARTSPlayerController::LoadCameraLocationWithIndex<4>);

	InputComponent->BindAction(TEXT("ShowConstructionProgressBars"), IE_Pressed, this,
	                           &ARTSPlayerController::StartShowingConstructionProgressBars);
	InputComponent->BindAction(TEXT("ShowConstructionProgressBars"), IE_Released, this,
	                           &ARTSPlayerController::StopShowingConstructionProgressBars);
	InputComponent->BindAction(TEXT("ShowHealthBars"), IE_Pressed, this, &ARTSPlayerController::StartShowingHealthBars);
	InputComponent->BindAction(TEXT("ShowHealthBars"), IE_Released, this, &ARTSPlayerController::StopShowingHealthBars);
	InputComponent->BindAction(TEXT("ShowProductionProgressBars"), IE_Pressed, this,
	                           &ARTSPlayerController::StartShowingProductionProgressBars);
	InputComponent->BindAction(TEXT("ShowProductionProgressBars"), IE_Released, this,
	                           &ARTSPlayerController::StopShowingProductionProgressBars);

	InputComponent->BindAction(TEXT("ConfirmBuildingPlacement"), IE_Released, this,
	                           &ARTSPlayerController::ConfirmBuildingPlacement);
	InputComponent->BindAction(TEXT("CancelBuildingPlacement"), IE_Released, this,
	                           &ARTSPlayerController::CancelBuildingPlacement);

	InputComponent->BindAction(TEXT("CancelConstruction"), IE_Released, this,
	                           &ARTSPlayerController::CancelConstruction);
	InputComponent->BindAction(TEXT("CancelProduction"), IE_Released, this, &ARTSPlayerController::CancelProduction);

	// Get camera bounds.
	for (TActorIterator<ARTSCameraBoundsVolume> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		if (ActorItr)
		{
			CameraBoundsVolume = *ActorItr;
			break;
		}
	}

	if (!CameraBoundsVolume)
	{
		UE_LOG(LogRTS, Warning, TEXT("No RTSCameraBoundsVolume found. Camera will be able to move anywhere."));
	}

	// Setup control groups and camera locations.
	ControlGroups.SetNum(10);
	CameraLocations.SetNum(5);
}

void ARTSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ARTSPlayerController::OnPlayerStateAvailable(ARTSPlayerState* NewPlayerState)
{
	// Discover own actors.
	ARTSPlayerState* RTSPlayerState = GetPlayerState();

	if (IsValid(RTSPlayerState))
	{
		RTSPlayerState->DiscoverOwnActors();
	}

	// Setup fog of war.
	ARTSGameState* GameState = Cast<ARTSGameState>(GetWorld()->GetGameState());

	if (IsValid(GameState))
	{
		ARTSVisionManager* VisionManager = GameState->GetVisionManager();

		if (IsValid(VisionManager))
		{
			VisionManager->SetLocalPlayerState(NewPlayerState);
		}
	}

	// Notify listeners.
	ReceiveOnPlayerStateAvailable(RTSPlayerState);
}

AActor* ARTSPlayerController::GetHoveredActor() const
{
	return HoveredActor;
}

ARTSPlayerState* ARTSPlayerController::GetPlayerState() const
{
	return Cast<ARTSPlayerState>(PlayerState);
}

TArray<AActor*> ARTSPlayerController::GetSelectedActors() const
{
	return SelectedActors;
}

bool ARTSPlayerController::GetObjectsAtScreenPosition(FVector2D ScreenPosition, TArray<FHitResult>& OutHitResults) const
{
	// Get ray.
	FVector WorldOrigin;
	FVector WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ScreenPosition, WorldOrigin, WorldDirection))
	{
		return false;
	}

	// Cast ray.
	return TraceObjects(WorldOrigin, WorldDirection, OutHitResults);
}

bool ARTSPlayerController::GetObjectsAtWorldPosition(const FVector& WorldPositionXY,
                                                     TArray<FHitResult>& OutHitResults) const
{
	// Get ray.
	FVector WorldOrigin = FVector(WorldPositionXY.X, WorldPositionXY.Y, HitResultTraceDistance / 2);
	FVector WorldDirection = -FVector::UpVector;

	// Cast ray.
	return TraceObjects(WorldOrigin, WorldDirection, OutHitResults);
}

bool ARTSPlayerController::GetSelectionFrame(FIntRect& OutSelectionFrame) const
{
	if (!bCreatingSelectionFrame)
	{
		return false;
	}

	// Get mouse input.
	float MouseX;
	float MouseY;

	if (!GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	float MinX = FMath::Min(SelectionFrameMouseStartPosition.X, MouseX);
	float MaxX = FMath::Max(SelectionFrameMouseStartPosition.X, MouseX);
	float MinY = FMath::Min(SelectionFrameMouseStartPosition.Y, MouseY);
	float MaxY = FMath::Max(SelectionFrameMouseStartPosition.Y, MouseY);

	OutSelectionFrame = FIntRect(FIntPoint(MinX, MinY), FIntPoint(MaxX, MaxY));

	return true;
}

ARTSTeamInfo* ARTSPlayerController::GetTeamInfo() const
{
	ARTSPlayerState* CurrentPlayerState = Cast<ARTSPlayerState>(PlayerState);

	if (CurrentPlayerState)
	{
		return CurrentPlayerState->GetTeam();
	}

	return nullptr;
}

bool ARTSPlayerController::IssueOrderToSelectedActors(const FRTSOrderData& Order)
{
	ERTSOrderGroupExecutionType GroupExecutionType = URTSOrderLibrary::GetOrderGroupExecutionType(Order.OrderClass);

	const bool bAppendToQueue = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

	// First pass: collect eligible pawns.
	TArray<APawn*> EligiblePawns;
	bool bMarkerNotified = false;

	for (auto SelectedActor : SelectedActors)
	{
		APawn* SelectedPawn = Cast<APawn>(SelectedActor);

		if (!IsValid(SelectedPawn))
		{
			continue;
		}

		if (SelectedPawn->GetOwner() != this)
		{
			continue;
		}

		if (!URTSOrderLibrary::CanObeyOrder(Order.OrderClass, SelectedActor, Order.Index))
		{
			continue;
		}

		FRTSOrderTargetData TargetData = URTSOrderLibrary::GetOrderTargetData(
			SelectedActor, Order.TargetActor, Order.TargetLocation);

		if (!URTSOrderLibrary::IsValidOrderTarget(Order.OrderClass, SelectedActor, TargetData, Order.Index))
		{
			continue;
		}

		if (GroupExecutionType == ERTSOrderGroupExecutionType::ORDERGROUPEXECUTION_Any)
		{
			// Just send a single actor.
			ServerIssueOrder(SelectedPawn, Order, bAppendToQueue);

		if (!IsNetMode(NM_DedicatedServer) && !bMarkerNotified)
		{
			// Notify listeners.
			NotifyOnIssuedOrder(SelectedPawn, Order);
			bMarkerNotified = true;
		}

			return true;
		}

		EligiblePawns.Add(SelectedPawn);
	}

	if (EligiblePawns.Num() == 0)
	{
		return false;
	}

	// Calculate formation offsets for location-targeted orders with multiple units.
	const bool bUseFormation = Order.OrderClass != nullptr
		&& Order.OrderClass->GetDefaultObject<URTSOrder>()->GetTargetType() == ERTSOrderTargetType::ORDERTARGET_Location
		&& EligiblePawns.Num() > 1;

	TArray<FVector> FormationPositions;
	if (bUseFormation)
	{
		FormationPositions = URTSFormationHelper::CalculateCircularFormation(
			Order.TargetLocation, EligiblePawns.Num(), FormationUnitSpacing);
	}

	// Second pass: dispatch orders with per-unit offsets.
	for (int32 i = 0; i < EligiblePawns.Num(); ++i)
	{
		APawn* Unit = EligiblePawns[i];

		FRTSOrderData PawnOrder = Order;
		if (bUseFormation && FormationPositions.IsValidIndex(i))
		{
			PawnOrder.TargetLocation = FormationPositions[i];
		}

		ServerIssueOrder(Unit, PawnOrder, bAppendToQueue);
	}

	// Notify once for the entire order, not per unit.
	if (!IsNetMode(NM_DedicatedServer) && EligiblePawns.Num() > 0)
	{
		NotifyOnIssuedOrder(EligiblePawns[0], Order);
		NotifyOnIssuedOrderVisualFeedback(Order);
	}

	return true;
}

void ARTSPlayerController::IssueDefaultOrderToActor(AActor* Actor, AActor* TargetActor, const FVector& TargetLocation)
{
	APawn* IssuedPawn = Cast<APawn>(Actor);

	if (!IsValid(IssuedPawn))
	{
		return;
	}

	for (TSubclassOf<URTSOrder> OrderClass : DefaultOrders)
	{
		if (!URTSOrderLibrary::CanObeyOrder(OrderClass, IssuedPawn, 0))
		{
			continue;
		}

		FRTSOrderTargetData TargetData = URTSOrderLibrary::GetOrderTargetData(IssuedPawn, TargetActor, TargetLocation);

		if (!URTSOrderLibrary::IsValidOrderTarget(OrderClass, IssuedPawn, TargetData, 0))
		{
			continue;
		}

		// Send order to server.
		FRTSOrderData Order;
		Order.OrderClass = OrderClass;
		Order.TargetActor = TargetActor;
		Order.TargetLocation = TargetLocation;

		ServerIssueOrder(IssuedPawn, Order, false);

		// Notify listeners (e.g. click marker, target circle).
		NotifyOnIssuedOrder(IssuedPawn, Order);
		return;
	}
}

void ARTSPlayerController::ServerIssueOrder_Implementation(APawn* OrderedPawn, const FRTSOrderData& Order, bool bAppendToQueue)
{
	if (!Order.OrderClass)
	{
		return;
	}

	// Route through the AI controller so order queuing is handled correctly.
	ARTSPawnAIController* AIController = Cast<ARTSPawnAIController>(OrderedPawn->GetController());
	if (IsValid(AIController))
	{
		AIController->IssueOrder(Order, bAppendToQueue);
	}
	else
	{
		FRTSOrderTargetData OrderTargetData;
		OrderTargetData.Actor = Order.TargetActor;
		OrderTargetData.Location = Order.TargetLocation;
		Order.OrderClass->GetDefaultObject<URTSOrder>()->IssueOrder(OrderedPawn, OrderTargetData, Order.Index);
	}

}

bool ARTSPlayerController::ServerIssueOrder_Validate(APawn* OrderedPawn, const FRTSOrderData& Order, bool bAppendToQueue)
{
	// Verify owner to prevent cheating.
	return OrderedPawn->GetOwner() == this;
}

bool ARTSPlayerController::GetObjectsAtPointerPosition(TArray<FHitResult>& OutHitResults) const
{
	// Get local player viewport.
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if (!LocalPlayer || !LocalPlayer->ViewportClient)
	{
		return false;
	}

	// Get mouse position.
	FVector2D MousePosition;
	if (!LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
	{
		return false;
	}

	return GetObjectsAtScreenPosition(MousePosition, OutHitResults);
}

bool ARTSPlayerController::GetObjectsInSelectionFrame(TArray<FHitResult>& HitResults) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	// Get selection frame.
	FIntRect SelectionFrame;

	if (!GetSelectionFrame(SelectionFrame))
	{
		return false;
	}

	if (SelectionFrame.Area() < 10)
	{
		// Selection frame too small - just consider left-click.
		return GetObjectsAtPointerPosition(HitResults);
	}

	// Iterate all actors.
	HitResults.Reset();

	for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;

		FVector2D ActorScreenPosition;

		if (UGameplayStatics::ProjectWorldToScreen(this, Actor->GetActorLocation(), ActorScreenPosition))
		{
			if (SelectionFrame.Contains(FIntPoint(ActorScreenPosition.X, ActorScreenPosition.Y)))
			{
				FHitResult HitResult(Actor, nullptr, Actor->GetActorLocation(), FVector());
				HitResults.Add(HitResult);
			}
		}
	}

	return HitResults.Num() > 0;
}

bool ARTSPlayerController::TraceObjects(const FVector& WorldOrigin, const FVector& WorldDirection,
                                        TArray<FHitResult>& OutHitResults) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	FCollisionObjectQueryParams Params(FCollisionObjectQueryParams::InitType::AllObjects);

	return World->LineTraceMultiByObjectType(
		OutHitResults,
		WorldOrigin,
		WorldOrigin + WorldDirection * HitResultTraceDistance,
		Params);
}

bool ARTSPlayerController::IsSelectableActor(AActor* Actor) const
{
	// Check if valid.
	if (!IsValid(Actor))
	{
		return false;
	}

	// Check if selectable.
	auto SelectableComponent = Actor->FindComponentByClass<URTSSelectableComponent>();

	if (!SelectableComponent)
	{
		return false;
	}

	return true;
}

void ARTSPlayerController::StartContinuousOrder()
{
	IssueDefaultOrderToSelectedActors();
	GetWorld()->GetTimerManager().SetTimer(ContinuousOrderTimerHandle, this,
		&ARTSPlayerController::IssueDefaultOrderToSelectedActors, 0.1f, true);
}

void ARTSPlayerController::StopContinuousOrder()
{
	GetWorld()->GetTimerManager().ClearTimer(ContinuousOrderTimerHandle);
}

void ARTSPlayerController::IssueDefaultOrderToSelectedActors()
{
	// Get objects at pointer position.
	TArray<FHitResult> HitResults;

	if (!GetObjectsAtPointerPosition(HitResults))
	{
		return;
	}

	IssueOrderTargetingObjectsToSelectedActors(HitResults);
}

void ARTSPlayerController::IssueOrderTargetingObjectsToSelectedActors(TArray<FHitResult>& HitResults)
{
	// Check if there's anybody to receive the order.
	if (SelectedActors.Num() == 0)
	{
		return;
	}

	// Try all default orders.
	for (auto& HitResult : HitResults)
	{
		if (HitResult.Actor != nullptr)
		{
			bool bIsIgnoredClass = false;

			for (TSubclassOf<AActor> IgnoredClass : DefaultOrderIgnoreTargetClasses)
			{
				if (HitResult.Actor->IsA(IgnoredClass))
				{
					bIsIgnoredClass = true;
					break;
				}
			}

			if (bIsIgnoredClass)
			{
				continue;
			}

			// Skip actors hidden by fog of war so terrain clicks near invisible enemies produce a move order.
			URTSVisibleComponent* VisibleComp = HitResult.Actor->FindComponentByClass<URTSVisibleComponent>();
			if (IsValid(VisibleComp) && !VisibleComp->IsVisibleForLocalClient())
			{
				continue;
			}

			for (TSubclassOf<URTSOrder> OrderClass : DefaultOrders)
			{
				FRTSOrderData Order;
				Order.OrderClass = OrderClass;
				Order.TargetActor = HitResult.Actor.Get();
				Order.TargetLocation = HitResult.Location;

				if (IssueOrderToSelectedActors(Order))
				{
					return;
				}
			}
		}
	}
}

bool ARTSPlayerController::GetSelectedSubgroupActorAndIndex(AActor** OutSelectedSubgroupActor,
                                                            int32* OutSelectedSubgroupActorIndex)
{
	*OutSelectedSubgroupActor = nullptr;
	*OutSelectedSubgroupActorIndex = -1;

	if (SelectedActors.Num() <= 0)
	{
		return false;
	}

	for (int32 Index = 0; Index < SelectedActors.Num(); ++Index)
	{
		AActor* SelectedActor = SelectedActors[Index];

		if (!IsValid(SelectedActors[Index]))
		{
			continue;
		}

		if (SelectedActor->GetClass() == GetSelectedSubgroup())
		{
			*OutSelectedSubgroupActor = SelectedActor;
			*OutSelectedSubgroupActorIndex = Index;
			return true;
		}
	}

	// Selected subgroup invalid.
	return false;
}

void ARTSPlayerController::SelectNextSubgroupInDirection(int32 Sign)
{
	// Find first actor in selected subgroup.
	AActor* OldSelectedSubgroupActor = nullptr;
	int32 OldSelectedSubgroupActorIndex = 0;

	if (!GetSelectedSubgroupActorAndIndex(&OldSelectedSubgroupActor, &OldSelectedSubgroupActorIndex))
	{
		SelectFirstSubgroup();
		return;
	}

	// Iterate all other selected actors, wrapping around.
	AActor* NewSelectedSubgroupActor = nullptr;
	int NewSelectedSubgroupActorIndex = OldSelectedSubgroupActorIndex + Sign;

	while (SelectedActors.IsValidIndex(NewSelectedSubgroupActorIndex))
	{
		if (SelectedActors[NewSelectedSubgroupActorIndex]->GetClass() != SelectedSubgroup)
		{
			NewSelectedSubgroupActor = SelectedActors[NewSelectedSubgroupActorIndex];
			break;
		}

		NewSelectedSubgroupActorIndex += Sign;
	}

	if (!IsValid(NewSelectedSubgroupActor))
	{
		NewSelectedSubgroupActorIndex = Sign >= 0 ? 0 : SelectedActors.Num() - 1;

		while (NewSelectedSubgroupActorIndex != OldSelectedSubgroupActorIndex)
		{
			if (SelectedActors[NewSelectedSubgroupActorIndex]->GetClass() != SelectedSubgroup)
			{
				NewSelectedSubgroupActor = SelectedActors[NewSelectedSubgroupActorIndex];
				break;
			}

			NewSelectedSubgroupActorIndex += Sign;
		}
	}

	// Select new subgroup.
	if (IsValid(NewSelectedSubgroupActor))
	{
		SelectSubgroup(NewSelectedSubgroupActor->GetClass());
	}
}

bool ARTSPlayerController::IssueAttackOrder(AActor* Target)
{
	FRTSOrderData AttackOrder;
	AttackOrder.OrderClass = URTSAttackOrder::StaticClass();
	AttackOrder.TargetActor = Target;

	return IssueOrderToSelectedActors(AttackOrder);
}

bool ARTSPlayerController::IssueBeginConstructionOrder(TSubclassOf<AActor> BuildingClass, const FVector& TargetLocation)
{
	// Determine index.
	int32 BuildingIndex = INDEX_NONE;

	for (auto SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor) || SelectedActor->GetOwner() != this)
		{
			continue;
		}

		BuildingIndex = URTSConstructionLibrary::GetConstructableBuildingIndex(SelectedActor, BuildingClass);

		if (BuildingIndex != INDEX_NONE)
		{
			break;
		}
	}

	if (BuildingIndex == INDEX_NONE)
	{
		return false;
	}

	FRTSOrderData BeginConstructionOrder;
	BeginConstructionOrder.OrderClass = URTSBeginConstructionOrder::StaticClass();
	BeginConstructionOrder.TargetLocation = TargetLocation;
	BeginConstructionOrder.Index = BuildingIndex;

	return IssueOrderToSelectedActors(BeginConstructionOrder);
}

bool ARTSPlayerController::IssueContinueConstructionOrder(AActor* ConstructionSite)
{
	FRTSOrderData ContinueConstructionOrder;
	ContinueConstructionOrder.OrderClass = URTSContinueConstructionOrder::StaticClass();
	ContinueConstructionOrder.TargetActor = ConstructionSite;

	return IssueOrderToSelectedActors(ContinueConstructionOrder);
}

bool ARTSPlayerController::IssueGatherOrder(AActor* ResourceSource)
{
	FRTSOrderData GatherOrder;
	GatherOrder.OrderClass = URTSGatherOrder::StaticClass();
	GatherOrder.TargetActor = ResourceSource;

	return IssueOrderToSelectedActors(GatherOrder);
}

bool ARTSPlayerController::IssueMoveOrder(const FVector& TargetLocation)
{
	FRTSOrderData MoveOrder;
	MoveOrder.OrderClass = URTSMoveOrder::StaticClass();
	MoveOrder.TargetLocation = TargetLocation;

	return IssueOrderToSelectedActors(MoveOrder);
}

AActor* ARTSPlayerController::GetSelectedProductionActorFor(TSubclassOf<AActor> ProductClass) const
{
	// Find suitable selected actor.
	for (auto SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		// Verify owner.
		if (SelectedActor->GetOwner() != this)
		{
			continue;
		}

		// Check if production actor.
		auto ProductionComponent = SelectedActor->FindComponentByClass<URTSProductionComponent>();

		if (!ProductionComponent)
		{
			continue;
		}

		if (!ProductionComponent->GetAvailableProducts().Contains(ProductClass))
		{
			continue;
		}

		return SelectedActor;
	}

	return nullptr;
}

bool ARTSPlayerController::CheckCanIssueProductionOrder(TSubclassOf<AActor> ProductClass)
{
	AActor* SelectedActor = GetSelectedProductionActorFor(ProductClass);

	if (!IsValid(SelectedActor))
	{
		return true;
	}

	// Check costs.
	URTSProductionCostComponent* ProductionCostComponent = URTSGameplayLibrary::FindDefaultComponentByClass<
		URTSProductionCostComponent>(ProductClass);

	if (ProductionCostComponent && !PlayerResourcesComponent->CanPayAllResources(
		ProductionCostComponent->GetResources()))
	{
		NotifyOnErrorOccurred(TEXT("Not enough resources."));
		return false;
	}

	// Check requirements.
	TSubclassOf<AActor> MissingRequirement;

	if (URTSGameplayLibrary::GetMissingRequirementFor(this, SelectedActor, ProductClass, MissingRequirement))
	{
		URTSNameComponent* NameComponent = URTSGameplayLibrary::FindDefaultComponentByClass<URTSNameComponent>(
			MissingRequirement);

		if (NameComponent)
		{
			FString ErrorMessage = TEXT("Missing requirement: ");
			ErrorMessage.Append(NameComponent->GetName().ToString());
			NotifyOnErrorOccurred(ErrorMessage);
		}
		else
		{
			NotifyOnErrorOccurred("Missing requirement.");
		}

		return false;
	}

	return true;
}

void ARTSPlayerController::IssueProductionOrder(TSubclassOf<AActor> ProductClass)
{
	AActor* SelectedActor = GetSelectedProductionActorFor(ProductClass);

	if (!IsValid(SelectedActor))
	{
		return;
	}

	// Begin production.
	ServerStartProduction(SelectedActor, ProductClass);

	if (IsNetMode(NM_Client))
	{
		UE_LOG(LogRTS, Log, TEXT("Ordered actor %s to start production %s."), *SelectedActor->GetName(),
		       *ProductClass->GetName());

		// Notify listeners.
		NotifyOnIssuedProductionOrder(SelectedActor, ProductClass);
	}
}

void ARTSPlayerController::IssueStopOrder()
{
	FRTSOrderData StopOrder;
	StopOrder.OrderClass = URTSStopOrder::StaticClass();

	IssueOrderToSelectedActors(StopOrder);
}

void ARTSPlayerController::SelectActors(TArray<AActor*> Actors, ERTSSelectionCameraFocusMode CameraFocusMode)
{
	// Check for double-selection.
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const float CurrentSelectionTime = World->GetRealTimeSeconds();
	const float SelectionTimeDelta = CurrentSelectionTime - LastSelectionTime;
	LastSelectionTime = CurrentSelectionTime;

	if (CameraFocusMode == ERTSSelectionCameraFocusMode::SELECTIONFOCUS_FocusOnDoubleSelection &&
		DoubleGroupSelectionTime > 0.0f && SelectionTimeDelta < DoubleGroupSelectionTime)
	{
		// Compare groups.
		if (Actors.Num() == SelectedActors.Num())
		{
			bool bSameSelection = true;

			for (AActor* Actor : Actors)
			{
				if (!SelectedActors.Contains(Actor))
				{
					bSameSelection = false;
					break;
				}
			}

			if (bSameSelection)
			{
				// Focus selected actors instead.
				if (SelectedActors.Num() > 0)
				{
					FocusCameraOnActor(SelectedActors[0]);
				}

				return;
			}
		}
	}

	// Clear selection.
	for (AActor* SelectedActor : SelectedActors)
	{
		URTSSelectableComponent* SelectableComponent = SelectedActor->FindComponentByClass<URTSSelectableComponent>();

		if (SelectableComponent)
		{
			SelectableComponent->DeselectActor();
		}
	}


	// Sort by priority and lifetime.
	Actors.Sort([=](const AActor& Lhs, const AActor& Rhs)
	{
		const URTSSelectableComponent* FirstSelectableComponent = Lhs.FindComponentByClass<URTSSelectableComponent>();
		const URTSSelectableComponent* SecondSelectableComponent = Rhs.FindComponentByClass<URTSSelectableComponent>();

		if (!IsValid(FirstSelectableComponent) || !IsValid(SecondSelectableComponent))
		{
			return true;
		}
		
		if (URTSGameplayLibrary::IsOwnedByLocalPlayer(&Lhs) && !URTSGameplayLibrary::IsOwnedByLocalPlayer(&Rhs))
		{
			return true;
		}
		
		if (!URTSGameplayLibrary::IsOwnedByLocalPlayer(&Lhs) && URTSGameplayLibrary::IsOwnedByLocalPlayer(&Rhs))
		{
			return false;
		}

		if (FirstSelectableComponent->GetSelectionPriority() < SecondSelectableComponent->GetSelectionPriority())
		{
			return true;
		}

		if (FirstSelectableComponent->GetSelectionPriority() > SecondSelectableComponent->GetSelectionPriority())
		{
			return false;
		}

		if (URTSGameplayLibrary::IsReadyToUse(&Lhs) && !URTSGameplayLibrary::IsReadyToUse(&Rhs))
		{
			return true;
		}

		if (!URTSGameplayLibrary::IsReadyToUse(&Lhs) && URTSGameplayLibrary::IsReadyToUse(&Rhs))
		{
			return false;
		}

		return Lhs.CreationTime < Rhs.CreationTime;
	});
	if (Actors.Num() > 0)
	{
		const int32 HighestPriority = Actors[0]->FindComponentByClass<URTSSelectableComponent>()->GetSelectionPriority();
		const bool IsControlledByPlayer = URTSGameplayLibrary::IsOwnedByLocalPlayer(Actors[0]);

		Actors.RemoveAll([HighestPriority, IsControlledByPlayer](AActor* Lhs) -> bool
		{
			const auto* SelectableComponent = Lhs->FindComponentByClass<URTSSelectableComponent>();

			if (URTSGameplayLibrary::IsOwnedByLocalPlayer(Lhs) != IsControlledByPlayer)
			{
				return true;
			}

			return SelectableComponent->GetSelectionPriority() > HighestPriority;
		});

		if (!IsControlledByPlayer && Actors.Num() > 1)
		{
			Actors.SetNum(1);
		}
	}

	// Apply new selection.
	SelectedActors = Actors;

	for (AActor* SelectedActor : SelectedActors)
	{
		URTSSelectableComponent* SelectableComponent = SelectedActor->FindComponentByClass<URTSSelectableComponent>();

		if (SelectableComponent)
		{
			SelectableComponent->SelectActor();

			// Play selection sound.
			if (SelectionSoundCooldownRemaining <= 0.0f &&
				URTSGameplayLibrary::IsOwnedByLocalPlayer(SelectedActor) &&
				IsValid(SelectableComponent->GetSelectedSound()))
			{
				UGameplayStatics::PlaySound2D(this, SelectableComponent->GetSelectedSound());
				SelectionSoundCooldownRemaining = SelectableComponent->GetSelectedSound()->GetDuration();
			}
		}
	}

	// Initially, select first subgroup.
	SelectFirstSubgroup();

	// Notify listeners.
	NotifyOnSelectionChanged(SelectedActors);
}

TSubclassOf<AActor> ARTSPlayerController::GetSelectedSubgroup() const
{
	return SelectedSubgroup;
}

AActor* ARTSPlayerController::GetSelectedSubgroupActor()
{
	AActor* SelectedSubgroupActor;
	int32 SelectedSubgroupActorIndex;
	return GetSelectedSubgroupActorAndIndex(&SelectedSubgroupActor, &SelectedSubgroupActorIndex)
		       ? SelectedSubgroupActor
		       : nullptr;
}

void ARTSPlayerController::GetSelectedSubgroupActors(TArray<AActor*>& OutActors) const
{
	for (AActor* Actor : SelectedActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetClass() == SelectedSubgroup)
		{
			OutActors.Add(Actor);
		}
	}
}

void ARTSPlayerController::SelectFirstSubgroup()
{
	if (SelectedActors.Num() > 0)
	{
		SelectSubgroup(SelectedActors[0]->GetClass());
	}
	else
	{
		SelectSubgroup(nullptr);
	}
}

void ARTSPlayerController::SelectNextSubgroup()
{
	SelectNextSubgroupInDirection(+1);
}

void ARTSPlayerController::SelectPreviousSubgroup()
{
	SelectNextSubgroupInDirection(-1);
}

void ARTSPlayerController::SelectSubgroup(TSubclassOf<AActor> NewSubgroup)
{
	if (SelectedSubgroup == NewSubgroup)
	{
		return;
	}

	SelectedSubgroup = NewSubgroup;

	if (NewSubgroup == nullptr)
	{
		UE_LOG(LogRTS, Verbose, TEXT("Cleared selected subgroup."));
	}
	else
	{
		UE_LOG(LogRTS, Verbose, TEXT("Selected subgroup %s."), *NewSubgroup->GetName());
	}

	// Notify listeners.
	NotifyOnSelectedSubgroupChanged(NewSubgroup);
}

void ARTSPlayerController::SaveControlGroup(int32 Index)
{
	if (Index < 0 || Index > 9)
	{
		return;
	}

	// Save control group.
	FRTSControlGroup ControlGroup;
	ControlGroup.Actors = SelectedActors;
	ControlGroups[Index] = ControlGroup;

	UE_LOG(LogRTS, Log, TEXT("Saved selection to control group %d."), Index);
}

void ARTSPlayerController::SaveControlGroup0() { SaveControlGroup(0); }
void ARTSPlayerController::SaveControlGroup1() { SaveControlGroup(1); }
void ARTSPlayerController::SaveControlGroup2() { SaveControlGroup(2); }
void ARTSPlayerController::SaveControlGroup3() { SaveControlGroup(3); }
void ARTSPlayerController::SaveControlGroup4() { SaveControlGroup(4); }
void ARTSPlayerController::SaveControlGroup5() { SaveControlGroup(5); }
void ARTSPlayerController::SaveControlGroup6() { SaveControlGroup(6); }
void ARTSPlayerController::SaveControlGroup7() { SaveControlGroup(7); }
void ARTSPlayerController::SaveControlGroup8() { SaveControlGroup(8); }
void ARTSPlayerController::SaveControlGroup9() { SaveControlGroup(9); }

void ARTSPlayerController::LoadControlGroup(int32 Index)
{
	if (Index < 0 || Index > 9)
	{
		return;
	}

	SelectActors(ControlGroups[Index].Actors, ERTSSelectionCameraFocusMode::SELECTIONFOCUS_FocusOnDoubleSelection);

	UE_LOG(LogRTS, Log, TEXT("Loaded selection from control group %d."), Index);
}

void ARTSPlayerController::LoadControlGroup0() { LoadControlGroup(0); }
void ARTSPlayerController::LoadControlGroup1() { LoadControlGroup(1); }
void ARTSPlayerController::LoadControlGroup2() { LoadControlGroup(2); }
void ARTSPlayerController::LoadControlGroup3() { LoadControlGroup(3); }
void ARTSPlayerController::LoadControlGroup4() { LoadControlGroup(4); }
void ARTSPlayerController::LoadControlGroup5() { LoadControlGroup(5); }
void ARTSPlayerController::LoadControlGroup6() { LoadControlGroup(6); }
void ARTSPlayerController::LoadControlGroup7() { LoadControlGroup(7); }
void ARTSPlayerController::LoadControlGroup8() { LoadControlGroup(8); }
void ARTSPlayerController::LoadControlGroup9() { LoadControlGroup(9); }

void ARTSPlayerController::FocusCameraOnLocation(FVector2D NewCameraLocation)
{
	APawn* PlayerPawn = GetPawnOrSpectator();

	if (!IsValid(PlayerPawn))
	{
		return;
	}

	// Calculate where to put the camera, considering its angle, to center on the specified location.
	FVector FinalCameraLocation = FVector(NewCameraLocation.X - GetCameraDistance(), NewCameraLocation.Y, 0.0f);

	// Enforce camera bounds.
	if (IsValid(CameraBoundsVolume) && !CameraBoundsVolume->EncompassesPoint(FinalCameraLocation))
	{
		UBrushComponent* CameraBoundsBrushComponent = CameraBoundsVolume->GetBrushComponent();
		FTransform CameraBoundsBrushTransform = CameraBoundsBrushComponent->GetComponentTransform();
		FBoxSphereBounds Bounds = CameraBoundsBrushComponent->CalcBounds(CameraBoundsBrushTransform);

		FinalCameraLocation.X = FMath::Clamp(FinalCameraLocation.X, Bounds.GetBox().Min.X, Bounds.GetBox().Max.X);
		FinalCameraLocation.Y = FMath::Clamp(FinalCameraLocation.Y, Bounds.GetBox().Min.Y, Bounds.GetBox().Max.Y);
	}

	// Keep camera height.
	FinalCameraLocation.Z = PlayerPawn->GetActorLocation().Z;

	// Update camera location.
	PlayerPawn->SetActorLocation(FinalCameraLocation);
}

void ARTSPlayerController::FocusCameraOnActor(AActor* Actor)
{
	TArray<AActor*> Actors;
	Actors.Add(Actor);
	FocusCameraOnActors(Actors);
}

void ARTSPlayerController::FocusCameraOnActors(TArray<AActor*> Actors)
{
	// Get center of group.
	FVector2D Locations = FVector2D::ZeroVector;
	int32 NumActors = 0;

	for (auto Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		FVector ActorLocation = Actor->GetActorLocation();
		Locations.X += ActorLocation.X;
		Locations.Y += ActorLocation.Y;

		++NumActors;
	}

	FVector2D Location = Locations / NumActors;
	FocusCameraOnLocation(Location);
}

void ARTSPlayerController::SaveCameraLocation(int32 Index)
{
	APawn* PlayerPawn = GetPawnOrSpectator();

	if (!IsValid(PlayerPawn))
	{
		return;
	}

	if (!CameraLocations.IsValidIndex(Index))
	{
		return;
	}

	// Save camera location.
	CameraLocations[Index] = PlayerPawn->GetActorLocation();

	UE_LOG(LogRTS, Log, TEXT("Saved camera location to index %d."), Index);
}

void ARTSPlayerController::LoadCameraLocation(int32 Index)
{
	APawn* PlayerPawn = GetPawnOrSpectator();

	if (!IsValid(PlayerPawn))
	{
		return;
	}

	if (!CameraLocations.IsValidIndex(Index))
	{
		return;
	}

	// Restore camera.
	PlayerPawn->SetActorLocation(CameraLocations[Index]);

	UE_LOG(LogRTS, Log, TEXT("Loaded camera location with index %d."), Index);
}

bool ARTSPlayerController::IsConstructionProgressBarHotkeyPressed() const
{
	return bConstructionProgressBarHotkeyPressed;
}

bool ARTSPlayerController::IsHealthBarHotkeyPressed() const
{
	return bHealthBarHotkeyPressed;
}

bool ARTSPlayerController::IsProductionProgressBarHotkeyPressed() const
{
	return bProductionProgressBarHotkeyPressed;
}

bool ARTSPlayerController::CheckCanBeginBuildingPlacement(TSubclassOf<AActor> BuildingClass)
{
	// Check resources.
	URTSConstructionSiteComponent* ConstructionSiteComponent = URTSGameplayLibrary::FindDefaultComponentByClass<
		URTSConstructionSiteComponent>(BuildingClass);

	if (ConstructionSiteComponent && !PlayerResourcesComponent->CanPayAllResources(
		ConstructionSiteComponent->GetConstructionCosts()))
	{
		NotifyOnErrorOccurred(TEXT("Not enough resources."));
		if (IsValid(BuildingPlacementFailedSound))
		{
			UGameplayStatics::PlaySound2D(this, BuildingPlacementFailedSound);
		}
		return false;
	}

	// Check requirements.
	if (SelectedActors.Num() > 0)
	{
		TSubclassOf<AActor> MissingRequirement;

		if (URTSGameplayLibrary::GetMissingRequirementFor(this, SelectedActors[0], BuildingClass, MissingRequirement))
		{
			URTSNameComponent* NameComponent = URTSGameplayLibrary::FindDefaultComponentByClass<URTSNameComponent>(
				MissingRequirement);

			if (NameComponent)
			{
				FString ErrorMessage = TEXT("Missing requirement: ");
				ErrorMessage.Append(NameComponent->GetName().ToString());
				NotifyOnErrorOccurred(ErrorMessage);
			}
			else
			{
				NotifyOnErrorOccurred("Missing requirement.");
			}

			if (IsValid(BuildingPlacementFailedSound))
			{
				UGameplayStatics::PlaySound2D(this, BuildingPlacementFailedSound);
			}
			return false;
		}
	}

	return true;
}

void ARTSPlayerController::BeginBuildingPlacement(TSubclassOf<AActor> BuildingClass)
{
	if (IsValid(BuildingCursor))
	{
		BuildingCursor->Destroy();
	}

	// Spawn preview building.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	BuildingCursor = GetWorld()->SpawnActor<ARTSBuildingCursor>(BuildingCursorClass, SpawnParams);

	if (IsValid(BuildingCursor))
	{
		BuildingCursor->SetupForBuilding(BuildingClass);
	}

	BuildingBeingPlacedClass = BuildingClass;

	UE_LOG(LogRTS, Log, TEXT("Beginning placement of building %s."), *BuildingClass->GetName());

	// Notify listeners.
	NotifyOnBuildingPlacementStarted(BuildingClass);
}

bool ARTSPlayerController::CanPlaceBuilding_Implementation(TSubclassOf<AActor> BuildingClass,
                                                           const FVector& Location) const
{
	if (IsValid(BuildingCursor) && BuildingCursor->HasGrid())
	{
		return BuildingCursor->AreAllCellsValid();
	}

	UWorld* World = GetWorld();
	return URTSCollisionLibrary::IsSuitableLocationForActor(World, BuildingClass, Location);
}

void ARTSPlayerController::Surrender()
{
	if (IsNetMode(NM_Client))
	{
		UE_LOG(LogRTS, Log, TEXT("%s surrenders the game."), *GetName());
	}

	ServerSurrender();
}

void ARTSPlayerController::GameHasEnded(class AActor* EndGameFocus /*= NULL*/, bool bIsWinner /*= false*/)
{
	ClientGameHasEnded(bIsWinner);
}

void ARTSPlayerController::ClientGameHasEnded_Implementation(bool bIsWinner)
{
	NotifyOnGameHasEnded(bIsWinner);
}

void ARTSPlayerController::StartSelectActors()
{
	if (BuildingCursor)
	{
		// We're selecting a building location instead.
		return;
	}

	// Get mouse input.
	float MouseX;
	float MouseY;

	if (GetMousePosition(MouseX, MouseY))
	{
		SelectionFrameMouseStartPosition = FVector2D(MouseX, MouseY);
		bCreatingSelectionFrame = true;
	}
}

void ARTSPlayerController::FinishSelectActors()
{
	// Don't process selection if the click is on the production queue UI.
	ARTSHUD* HUD = Cast<ARTSHUD>(GetHUD());
	if (HUD)
	{
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY) && HUD->IsPositionOnProductionQueue(MouseX, MouseY))
		{
			bCreatingSelectionFrame = false;
			return;
		}
	}

	// Get objects at pointer position.
	TArray<FHitResult> HitResults;

	if (!GetObjectsInSelectionFrame(HitResults))
	{
		bCreatingSelectionFrame = false;
		return;
	}

	// Check results.
	TArray<AActor*> ActorsToSelect;

	if (bAddSelectionHotkeyPressed || bToggleSelectionHotkeyPressed)
	{
		ActorsToSelect = SelectedActors;
	}

	for (auto& HitResult : HitResults)
	{
		if (!IsSelectableActor(HitResult.Actor.Get()))
		{
			continue;
		}

		// Check how to apply selection.
		if (bToggleSelectionHotkeyPressed)
		{
			if (SelectedActors.Contains(HitResult.Actor))
			{
				// Deselect actor.
				ActorsToSelect.Remove(HitResult.Actor.Get());

				UE_LOG(LogRTS, Log, TEXT("Deselected actor %s."), *HitResult.Actor->GetName());
			}
			else if (!ActorsToSelect.Contains(HitResult.Actor))
			{
				// Select actor.
				ActorsToSelect.Add(HitResult.Actor.Get());

				UE_LOG(LogRTS, Log, TEXT("Selected actor %s."), *HitResult.Actor->GetName());
			}
		}
		else
		{
			if (ActorsToSelect.Contains(HitResult.Actor))
			{
				continue;
			}

			// Select actor.
			ActorsToSelect.Add(HitResult.Actor.Get());

			UE_LOG(LogRTS, Log, TEXT("Selected actor %s."), *HitResult.Actor->GetName());
		}
	}

	SelectActors(ActorsToSelect, ERTSSelectionCameraFocusMode::SELECTIONFOCUS_DoNothing);

	bCreatingSelectionFrame = false;
}

void ARTSPlayerController::StartShowingConstructionProgressBars()
{
	bConstructionProgressBarHotkeyPressed = true;
}

void ARTSPlayerController::StopShowingConstructionProgressBars()
{
	bConstructionProgressBarHotkeyPressed = false;
}

void ARTSPlayerController::StartShowingHealthBars()
{
	bHealthBarHotkeyPressed = true;
}

void ARTSPlayerController::StopShowingHealthBars()
{
	bHealthBarHotkeyPressed = false;
}

void ARTSPlayerController::StartShowingProductionProgressBars()
{
	bProductionProgressBarHotkeyPressed = true;
}

void ARTSPlayerController::StopShowingProductionProgressBars()
{
	bProductionProgressBarHotkeyPressed = false;
}

void ARTSPlayerController::StartAddSelection()
{
	bAddSelectionHotkeyPressed = true;
}

void ARTSPlayerController::StopAddSelection()
{
	bAddSelectionHotkeyPressed = false;
}

void ARTSPlayerController::StartToggleSelection()
{
	bToggleSelectionHotkeyPressed = true;
}

void ARTSPlayerController::StopToggleSelection()
{
	bToggleSelectionHotkeyPressed = false;
}

void ARTSPlayerController::ConfirmBuildingPlacement()
{
	if (!BuildingCursor)
	{
		return;
	}

	if (!CanPlaceBuilding(BuildingBeingPlacedClass, HoveredWorldPosition))
	{
		UE_LOG(LogRTS, Log, TEXT("Can't place building %s at %s."), *BuildingBeingPlacedClass->GetName(),
		       *HoveredWorldPosition.ToString());

		// Notify listeners.
		NotifyOnBuildingPlacementError(BuildingBeingPlacedClass, HoveredWorldPosition);
		NotifyOnErrorOccurred(TEXT("Can't place building here."));

		// Play failure sound: prefer the selected builder's ConstructionFailedSounds, fall back to the generic UI sound.
		bool bBuilderSoundPlayed = false;
		for (AActor* SelectedActor : SelectedActors)
		{
			URTSBuilderComponent* BuilderComponent = SelectedActor ? SelectedActor->FindComponentByClass<URTSBuilderComponent>() : nullptr;
			if (BuilderComponent && BuilderComponent->GetConstructionFailedSounds().Num() > 0)
			{
				TArray<USoundBase*> Sounds = BuilderComponent->GetConstructionFailedSounds();
				int32 Index = FMath::RandRange(0, Sounds.Num() - 1);
				UGameplayStatics::PlaySoundAtLocation(this, Sounds[Index], SelectedActor->GetActorLocation());
				bBuilderSoundPlayed = true;
				break;
			}
		}
		if (!bBuilderSoundPlayed && IsValid(BuildingPlacementFailedSound))
		{
			UGameplayStatics::PlaySound2D(this, BuildingPlacementFailedSound);
		}
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("Placed building %s at %s."), *BuildingBeingPlacedClass->GetName(),
	       *HoveredWorldPosition.ToString());

	// Remove dummy building.
	BuildingCursor->Destroy();
	BuildingCursor = nullptr;

	// Push own-team units out of the build zone immediately so they clear the area while the builder walks there.
	{
		UShapeComponent* BuildingShape =
			URTSGameplayLibrary::FindDefaultComponentByClass<UShapeComponent>(BuildingBeingPlacedClass);

		if (BuildingShape)
		{
			FCollisionObjectQueryParams OverlapParams(FCollisionObjectQueryParams::AllDynamicObjects);
			TArray<FOverlapResult> Overlaps;
			GetWorld()->OverlapMultiByObjectType(
				Overlaps, HoveredWorldPosition, FQuat::Identity,
				OverlapParams, BuildingShape->GetCollisionShape());

			float BuildingRadius = URTSCollisionLibrary::GetCollisionSize(BuildingBeingPlacedClass) / 2.0f;

			for (const FOverlapResult& Overlap : Overlaps)
			{
				AActor* Actor = Overlap.Actor.Get();
				if (!Actor || !Actor->IsA<APawn>())
				{
					continue;
				}

				URTSOwnerComponent* OwnerComp = Actor->FindComponentByClass<URTSOwnerComponent>();
				if (!OwnerComp || !OwnerComp->IsSameTeamAsController(this))
				{
					continue;
				}

				ARTSPawnAIController* UnitController =
					Cast<ARTSPawnAIController>(Cast<APawn>(Actor)->GetController());
				if (!UnitController)
				{
					continue;
				}

				FVector ToUnit = Actor->GetActorLocation() - HoveredWorldPosition;
				ToUnit.Z = 0.0f;
				FVector ExitDirection = ToUnit.IsNearlyZero()
					? FVector(1.0f, 0.0f, 0.0f)
					: ToUnit.GetSafeNormal();
				float UnitRadius = URTSCollisionLibrary::GetActorCollisionSize(Actor) / 2.0f;
				FVector ExitLocation = HoveredWorldPosition
					+ ExitDirection * (BuildingRadius + UnitRadius + 50.0f);
				ExitLocation.Z = Actor->GetActorLocation().Z;

				UnitController->IssueMoveOrder(ExitLocation);
			}
		}
	}

	// Notify listeners.
	NotifyOnBuildingPlacementConfirmed(BuildingBeingPlacedClass, HoveredWorldPosition);

	// Start construction.
	IssueBeginConstructionOrder(BuildingBeingPlacedClass, HoveredWorldPosition);
}

void ARTSPlayerController::CancelBuildingPlacement()
{
	if (!BuildingCursor)
	{
		return;
	}

	// Remove dummy building.
	BuildingCursor->Destroy();
	BuildingCursor = nullptr;

	UE_LOG(LogRTS, Log, TEXT("Cancelled placement of building %s."), *BuildingBeingPlacedClass->GetName());

	// Notify listeners.
	NotifyOnBuildingPlacementCancelled(BuildingBeingPlacedClass);
}

void ARTSPlayerController::CancelConstruction()
{
	for (auto SelectedActor : SelectedActors)
	{
		// Verify construction site and owner.
		auto ConstructionSiteComponent = SelectedActor->FindComponentByClass<URTSConstructionSiteComponent>();

		if (!ConstructionSiteComponent)
		{
			continue;
		}

		if (SelectedActor->GetOwner() != this)
		{
			continue;
		}

		// Send message to server.
		ServerCancelConstruction(SelectedActor);

		// Only cancel one construction at a time.
		return;
	}
}

void ARTSPlayerController::ServerCancelConstruction_Implementation(AActor* ConstructionSite)
{
	auto ConstructionSiteComponent = ConstructionSite->FindComponentByClass<URTSConstructionSiteComponent>();

	if (!ConstructionSiteComponent)
	{
		return;
	}

	// Cancel construction.
	ConstructionSiteComponent->CancelConstruction();
}

bool ARTSPlayerController::ServerCancelConstruction_Validate(AActor* ConstructionSite)
{
	// Verify owner to prevent cheating.
	return ConstructionSite->GetOwner() == this;
}

void ARTSPlayerController::CancelProduction()
{
	for (auto SelectedActor : SelectedActors)
	{
		// Verify production actor.
		auto ProductionComponent = SelectedActor->FindComponentByClass<URTSProductionComponent>();

		if (!ProductionComponent)
		{
			continue;
		}

		if (SelectedActor->GetOwner() != this)
		{
			continue;
		}

		if (!URTSGameplayLibrary::IsReadyToUse(SelectedActor))
		{
			continue;
		}

		// Send message to server.
		ServerCancelProduction(SelectedActor);

		// Only cancel one production at a time.
		return;
	}
}

void ARTSPlayerController::ServerCancelProduction_Implementation(AActor* ProductionActor)
{
	auto ProductionComponent = ProductionActor->FindComponentByClass<URTSProductionComponent>();

	if (!ProductionComponent)
	{
		return;
	}

	// Cancel production.
	ProductionComponent->CancelProduction();
}

bool ARTSPlayerController::ServerCancelProduction_Validate(AActor* ProductionActor)
{
	// Verify owner to prevent cheating.
	return ProductionActor->GetOwner() == this;
}

void ARTSPlayerController::RequestCancelProductionAt(AActor* ProductionActor, int32 QueueIndex, int32 ProductIndex)
{
	if (!IsValid(ProductionActor))
	{
		return;
	}

	ServerCancelProductionAt(ProductionActor, QueueIndex, ProductIndex);
}

void ARTSPlayerController::ServerCancelProductionAt_Implementation(AActor* ProductionActor, int32 QueueIndex, int32 ProductIndex)
{
	auto ProductionComponent = ProductionActor->FindComponentByClass<URTSProductionComponent>();

	if (!ProductionComponent)
	{
		return;
	}

	ProductionComponent->CancelProduction(QueueIndex, ProductIndex);
}

bool ARTSPlayerController::ServerCancelProductionAt_Validate(AActor* ProductionActor, int32 QueueIndex, int32 ProductIndex)
{
	return IsValid(ProductionActor) && ProductionActor->GetOwner() == this;
}

void ARTSPlayerController::ServerStartProduction_Implementation(AActor* ProductionActor,
                                                                TSubclassOf<AActor> ProductClass)
{
	auto ProductionComponent = ProductionActor->FindComponentByClass<URTSProductionComponent>();

	if (!IsValid(ProductionComponent))
	{
		return;
	}

	ProductionComponent->StartProduction(ProductClass);
}

bool ARTSPlayerController::ServerStartProduction_Validate(AActor* ProductionActor, TSubclassOf<AActor> ProductClass)
{
	// Verify owner to prevent cheating.
	return ProductionActor->GetOwner() == this;
}

void ARTSPlayerController::ServerSurrender_Implementation()
{
	UE_LOG(LogRTS, Log, TEXT("%s surrenders the game."), *GetName());

	// Notify game mode.
	ARTSGameMode* GameMode = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));

	if (GameMode != nullptr)
	{
		GameMode->NotifyOnPlayerDefeated(this);
	}
}

bool ARTSPlayerController::ServerSurrender_Validate()
{
	return true;
}

void ARTSPlayerController::MoveCameraLeftRight(float Value)
{
	CameraLeftRightAxisValue = Value;
}

void ARTSPlayerController::MoveCameraUpDown(float Value)
{
	CameraUpDownAxisValue = Value;
}

void ARTSPlayerController::ZoomCamera(float Value)
{
	CameraZoomAxisValue = Value;
}

void ARTSPlayerController::StartRotateCamera()
{
	bRotatingCamera = true;
}

void ARTSPlayerController::StopRotateCamera()
{
	bRotatingCamera = false;
}

void ARTSPlayerController::RotateCameraPitch(float Value)
{
	if (bRotatingCamera)
	{
		CameraPitchAxisValue = Value;
	}
	else
	{
		CameraPitchAxisValue = 0.0f;
	}
}

float ARTSPlayerController::GetCameraDistance() const
{
	APawn* PlayerPawn = GetPawnOrSpectator();

	if (!IsValid(PlayerPawn))
	{
		return 0.0f;
	}

	UCameraComponent* Camera = PlayerPawn->FindComponentByClass<UCameraComponent>();

	if (!IsValid(Camera))
	{
		return 0.0f;
	}

	// Get camera angle.
	float CameraAngle = Camera->GetRelativeRotation().Pitch;

	if (CameraAngle < 0.0f)
	{
		CameraAngle += 90.0f;
	}

	// Get camera distance using trigonometry.
	// We are assuming that the terrain is flat, centered at the origin, and the camera has no roll or yaw.
	return Camera->GetRelativeLocation().Z * FMath::Tan(FMath::DegreesToRadians(CameraAngle));
}

void ARTSPlayerController::NotifyOnActorOwnerChanged(AActor* Actor)
{
	ReceiveOnActorOwnerChanged(Actor);
}

void ARTSPlayerController::NotifyOnBuildingPlacementStarted(TSubclassOf<AActor> BuildingClass)
{
	ReceiveOnBuildingPlacementStarted(BuildingClass);
}

void ARTSPlayerController::NotifyOnBuildingPlacementConfirmed(TSubclassOf<AActor> BuildingClass,
                                                              const FVector& Location)
{
	ReceiveOnBuildingPlacementConfirmed(BuildingClass, Location);
}

void ARTSPlayerController::NotifyOnBuildingPlacementError(TSubclassOf<AActor> BuildingClass, const FVector& Location)
{
	ReceiveOnBuildingPlacementError(BuildingClass, Location);
}

void ARTSPlayerController::NotifyOnBuildingPlacementCancelled(TSubclassOf<AActor> BuildingClass)
{
	ReceiveOnBuildingPlacementCancelled(BuildingClass);
}

void ARTSPlayerController::NotifyOnErrorOccurred(const FString& ErrorMessage)
{
	ReceiveOnErrorOccurred(ErrorMessage);
}

void ARTSPlayerController::NotifyOnGameHasEnded(bool bIsWinner)
{
	ReceiveOnGameHasEnded(bIsWinner);
}

void ARTSPlayerController::NotifyOnIssuedOrder(APawn* OrderedPawn, const FRTSOrderData& Order)
{
	ReceiveOnIssuedOrder(OrderedPawn, Order);

	if (Order.OrderClass == URTSAttackOrder::StaticClass())
	{
		NotifyOnIssuedAttackOrder(OrderedPawn, Order.TargetActor);
	}
	else if (Order.OrderClass == URTSBeginConstructionOrder::StaticClass())
	{
		TSubclassOf<AActor> BuildingClass = URTSConstructionLibrary::GetConstructableBuildingClass(
			OrderedPawn, Order.Index);
		NotifyOnIssuedBeginConstructionOrder(OrderedPawn, BuildingClass, Order.TargetLocation);
	}
	else if (Order.OrderClass == URTSContinueConstructionOrder::StaticClass())
	{
		NotifyOnIssuedContinueConstructionOrder(OrderedPawn, Order.TargetActor);
	}
	else if (Order.OrderClass == URTSGatherOrder::StaticClass())
	{
		NotifyOnIssuedGatherOrder(OrderedPawn, Order.TargetActor);
	}
	else if (Order.OrderClass == URTSMoveOrder::StaticClass())
	{
		NotifyOnIssuedMoveOrder(OrderedPawn, Order.TargetLocation);
	}
	else if (Order.OrderClass == URTSStopOrder::StaticClass())
	{
		NotifyOnIssuedStopOrder(OrderedPawn);
	}
	else if (Order.OrderClass == URTSSetRallyPointToLocationOrder::StaticClass())
	{
		NotifyOnIssuedSetRallyPoint(OrderedPawn, Order.TargetLocation);
	}
	else if (Order.OrderClass == URTSSetRallyPointToActorOrder::StaticClass())
	{
		FVector TargetLoc = IsValid(Order.TargetActor) ?
			Order.TargetActor->GetActorLocation() : Order.TargetLocation;
		NotifyOnIssuedSetRallyPoint(OrderedPawn, TargetLoc);
	}
}

void ARTSPlayerController::NotifyOnIssuedAttackOrder(APawn* OrderedPawn, AActor* Target)
{
	ReceiveOnIssuedAttackOrder(OrderedPawn, Target);
}

void ARTSPlayerController::NotifyOnIssuedBeginConstructionOrder(APawn* OrderedPawn, TSubclassOf<AActor> BuildingClass,
                                                                const FVector& TargetLocation)
{
	ReceiveOnIssuedBeginConstructionOrder(OrderedPawn, BuildingClass, TargetLocation);
}

void ARTSPlayerController::NotifyOnIssuedContinueConstructionOrder(APawn* OrderedPawn, AActor* ConstructionSite)
{
	ReceiveOnIssuedContinueConstructionOrder(OrderedPawn, ConstructionSite);
}

void ARTSPlayerController::NotifyOnIssuedGatherOrder(APawn* OrderedPawn, AActor* ResourceSource)
{
	ReceiveOnIssuedGatherOrder(OrderedPawn, ResourceSource);
}

void ARTSPlayerController::NotifyOnIssuedMoveOrder(APawn* OrderedPawn, const FVector& TargetLocation)
{
	ReceiveOnIssuedMoveOrder(OrderedPawn, TargetLocation);
}

void ARTSPlayerController::NotifyOnIssuedOrderVisualFeedback(const FRTSOrderData& Order)
{
	if (Order.OrderClass == URTSAttackOrder::StaticClass())
	{
		if (IsValid(Order.TargetActor) && EnemyOrderEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				EnemyOrderEffect,
				Order.TargetActor->GetRootComponent(),
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				true);
		}
	}
	else if (Order.OrderClass == URTSGatherOrder::StaticClass())
	{
		if (IsValid(Order.TargetActor) && ResourceOrderEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ResourceOrderEffect,
				Order.TargetActor->GetActorLocation(),
				FRotator::ZeroRotator);
		}
	}
}

void ARTSPlayerController::NotifyOnIssuedSetRallyPoint(AActor* OrderedActor, const FVector& TargetLocation)
{
    // Destroy previous indicator.
    if (IsValid(ActiveRallyPointIndicator))
    {
        ActiveRallyPointIndicator->Destroy();
        ActiveRallyPointIndicator = nullptr;
    }

    if (IsValid(OrderedActor) && RallyPointTravelEffect)
    {
        FVector StartLocation = OrderedActor->GetActorLocation();

        ActiveRallyPointIndicator = GetWorld()->SpawnActor<ARTSRallyPointIndicator>(
            ARTSRallyPointIndicator::StaticClass(),
            StartLocation,
            FRotator::ZeroRotator);

        if (IsValid(ActiveRallyPointIndicator))
        {
            ActiveRallyPointIndicator->Initialize(
                RallyPointTravelEffect,
                RallyPointArrivalEffect,
                StartLocation,
                TargetLocation,
                OrderedActor);
        }
    }

    ReceiveOnIssuedSetRallyPoint(OrderedActor, TargetLocation);
}

void ARTSPlayerController::NotifyOnIssuedProductionOrder(AActor* OrderedActor, TSubclassOf<AActor> ProductClass)
{
	ReceiveOnIssuedProductionOrder(OrderedActor, ProductClass);
}

void ARTSPlayerController::NotifyOnIssuedStopOrder(APawn* OrderedPawn)
{
	ReceiveOnIssuedStopOrder(OrderedPawn);
}

void ARTSPlayerController::NotifyOnSelectionChanged(const TArray<AActor*>& Selection)
{
	ReceiveOnSelectionChanged(Selection);
}

void ARTSPlayerController::NotifyOnTeamChanged(ARTSTeamInfo* NewTeam)
{
	if (NewTeam)
	{
		// Notify listeners that new vision info is available now.
		ARTSVisionInfo* VisionInfo = ARTSVisionInfo::GetVisionInfoForTeam(GetWorld(), NewTeam->GetTeamIndex());

		if (IsValid(VisionInfo))
		{
			NotifyOnVisionInfoAvailable(VisionInfo);
		}
	}
}

void ARTSPlayerController::NotifyOnVisionInfoAvailable(ARTSVisionInfo* VisionInfo)
{
	// On server side, we're only interested in our own vision info.
	// Other player controllers that exist on the server for replication to the clients
	// should not affect rendering vision for the local player.
	if (this != GetWorld()->GetFirstPlayerController())
	{
		return;
	}

	if (!IsValid(VisionInfo))
	{
		return;
	}

	// Setup fog of war.
	ARTSGameState* GameState = Cast<ARTSGameState>(GetWorld()->GetGameState());

	if (IsValid(GameState))
	{
		ARTSVisionManager* VisionManager = GameState->GetVisionManager();

		if (IsValid(VisionManager))
		{
			VisionManager->SetLocalVisionInfo(VisionInfo);
		}
	}

	// Allow others to setup vision.
	ReceiveOnVisionInfoAvailable(VisionInfo);
}

void ARTSPlayerController::NotifyOnMinimapClicked(const FPointerEvent& InMouseEvent, const FVector2D& MinimapPosition,
                                                  const FVector& WorldPosition)
{
	APawn* PlayerPawn = GetPawnOrSpectator();

	if (!PlayerPawn)
	{
		return;
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		// Move camera.
		FVector NewCameraLocation = FVector(WorldPosition.X, WorldPosition.Y, PlayerPawn->GetActorLocation().Z);
		PlayerPawn->SetActorLocation(NewCameraLocation);
	}
	else if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		// Get objects at pointer position.
		TArray<FHitResult> HitResults;

		if (!GetObjectsAtWorldPosition(WorldPosition, HitResults))
		{
			return;
		}

		IssueOrderTargetingObjectsToSelectedActors(HitResults);
	}

	// Notify listeners.
	ReceiveOnMinimapClicked(InMouseEvent, MinimapPosition, WorldPosition);
}

void ARTSPlayerController::NotifyOnSelectedSubgroupChanged(TSubclassOf<AActor> Subgroup)
{
	ReceiveOnSelectedSubgroupChanged(Subgroup);
}

void ARTSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Update sound cooldowns.
	if (SelectionSoundCooldownRemaining > 0.0f)
	{
		SelectionSoundCooldownRemaining -= DeltaTime;
	}

	APawn* PlayerPawn = GetPawnOrSpectator();

	if (!PlayerPawn)
	{
		return;
	}

	// Get mouse input.
	float MouseX;
	float MouseY;

	const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());

	const float ScrollBorderRight = ViewportSize.X - CameraScrollThreshold;
	const float ScrollBorderTop = ViewportSize.Y - CameraScrollThreshold;

	if (GetMousePosition(MouseX, MouseY))
	{
		if (MouseX <= CameraScrollThreshold)
		{
			float T = 1.0f - (MouseX / CameraScrollThreshold);
			CameraLeftRightAxisValue -= T * T * EdgeScrollSpeedMultiplier;
		}
		else if (MouseX >= ScrollBorderRight)
		{
			float T = (MouseX - ScrollBorderRight) / CameraScrollThreshold;
			CameraLeftRightAxisValue += T * T * EdgeScrollSpeedMultiplier;
		}

		if (MouseY <= CameraScrollThreshold)
		{
			float T = 1.0f - (MouseY / CameraScrollThreshold);
			CameraUpDownAxisValue += T * T * EdgeScrollSpeedMultiplier;
		}
		else if (MouseY >= ScrollBorderTop)
		{
			float T = (MouseY - ScrollBorderTop) / CameraScrollThreshold;
			CameraUpDownAxisValue -= T * T * EdgeScrollSpeedMultiplier;
		}
	}

	// Apply input.
	CameraLeftRightAxisValue = FMath::Clamp(CameraLeftRightAxisValue, -EdgeScrollSpeedMultiplier, +EdgeScrollSpeedMultiplier);
	CameraUpDownAxisValue = FMath::Clamp(CameraUpDownAxisValue, -EdgeScrollSpeedMultiplier, +EdgeScrollSpeedMultiplier);

	FVector Location = PlayerPawn->GetActorLocation();
	Location += FVector::RightVector * CameraSpeed * CameraLeftRightAxisValue * DeltaTime;
	Location += FVector::ForwardVector * CameraSpeed * CameraUpDownAxisValue * DeltaTime;

	// Enforce camera bounds.
	if (!CameraBoundsVolume || CameraBoundsVolume->EncompassesPoint(Location))
	{
		PlayerPawn->SetActorLocation(Location);
	}

	// Apply zoom input.
	UCameraComponent* PlayerPawnCamera = PlayerPawn->FindComponentByClass<UCameraComponent>();

	if (IsValid(PlayerPawnCamera))
	{
		FVector CameraLocation = PlayerPawnCamera->GetRelativeLocation();
		CameraLocation.Z += CameraZoomSpeed * CameraZoomAxisValue * DeltaTime;
		CameraLocation.Z = FMath::Clamp(CameraLocation.Z, MinCameraDistance, MaxCameraDistance);
		PlayerPawnCamera->SetRelativeLocation(CameraLocation);

		// Apply pitch rotation input.
		if (CameraPitchAxisValue != 0.0f)
		{
			FRotator CameraRotation = PlayerPawnCamera->GetRelativeRotation();
			CameraRotation.Pitch += CameraPitchSpeed * CameraPitchAxisValue * DeltaTime;
			CameraRotation.Pitch = FMath::Clamp(CameraRotation.Pitch, MinCameraPitch, MaxCameraPitch);
			PlayerPawnCamera->SetRelativeRotation(CameraRotation);
		}
	}

	// Get hovered actors.
	AActor* OldHoveredActor = HoveredActor;
	HoveredActor = nullptr;

	TArray<FHitResult> HitResults;

	if (GetObjectsAtPointerPosition(HitResults))
	{
		for (auto& HitResult : HitResults)
		{
			// Store hovered world position.
			HoveredWorldPosition = HitResult.Location;

			// Update position of building being placed.
			if (BuildingCursor)
			{
				BuildingCursor->SetCursorLocation(HoveredWorldPosition);

				bool bLocationValid = CanPlaceBuilding(BuildingBeingPlacedClass, HoveredWorldPosition);
				BuildingCursor->SetLocationValid(bLocationValid);
			}

			// Check if hit any actor.
			if (HitResult.Actor == nullptr || Cast<ALandscape>(HitResult.Actor.Get()) != nullptr)
			{
				continue;
			}

			// Check if hit selectable actor.
			auto SelectableComponent = HitResult.Actor->FindComponentByClass<URTSSelectableComponent>();

			if (!SelectableComponent)
			{
				continue;
			}

			// Set hovered actor.
			HoveredActor = HitResult.Actor.Get();
		}
	}

	// Verify selection.
	int32 OldNumSelectedActors = SelectedActors.Num();

	for (int32 SelectedActorIndex = SelectedActors.Num() - 1; SelectedActorIndex >= 0; --SelectedActorIndex)
	{
		AActor* SelectedActor = SelectedActors[SelectedActorIndex];

		if (!IsValid(SelectedActor))
		{
			// Remove invalid actors.
			SelectedActors.RemoveAt(SelectedActorIndex);
			continue;
		}

		if (!URTSGameplayLibrary::IsFullyVisibleForLocalClient(SelectedActor))
		{
			// Remove invisible actors.
			SelectedActors.RemoveAt(SelectedActorIndex);

			// Update selection effects.
			URTSSelectableComponent* SelectableComponent = SelectedActor->FindComponentByClass<
				URTSSelectableComponent>();

			if (IsValid(SelectableComponent))
			{
				SelectableComponent->DeselectActor();
			}

			continue;
		}
	}

	if (SelectedActors.Num() != OldNumSelectedActors)
	{
		// Notify listeners.
		NotifyOnSelectionChanged(SelectedActors);

		// Verify subgroup.
		if (SelectedActors.Num() > 0 && !IsValid(GetSelectedSubgroupActor()))
		{
			SelectFirstSubgroup();
		}
	}

	// Notify listeners.
	if (OldHoveredActor != HoveredActor)
	{
		if (IsValid(OldHoveredActor))
		{
			auto OldHoveredSelectableComponent = OldHoveredActor->FindComponentByClass<URTSSelectableComponent>();

			if (OldHoveredSelectableComponent)
			{
				OldHoveredSelectableComponent->UnhoverActor();
			}
		}

		if (IsValid(HoveredActor))
		{
			auto HoveredSelectableComponent = HoveredActor->FindComponentByClass<URTSSelectableComponent>();

			if (HoveredSelectableComponent)
			{
				HoveredSelectableComponent->HoverActor();
			}
		}
	}
}

void ARTSPlayerController::InitPlayerState()
{
	Super::InitPlayerState();

	if (!IsValid(PlayerState))
	{
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("Player %s set up player state %s (%s)."), *GetName(), *PlayerState->GetName(),
	       *PlayerState->GetPlayerName());

	OnPlayerStateAvailable(GetPlayerState());
}

void ARTSPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!IsValid(PlayerState))
	{
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("Player %s received player state %s (%s)."), *GetName(), *PlayerState->GetName(),
	       *PlayerState->GetPlayerName());

	OnPlayerStateAvailable(GetPlayerState());
}
