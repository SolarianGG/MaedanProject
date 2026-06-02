#include "RTSPawnAIController.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"

#include "RTSGameplayTagsComponent.h"
#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "Economy/RTSGathererComponent.h"
#include "Combat/RTSAttackComponent.h"
#include "Libraries/RTSConstructionLibrary.h"
#include "Libraries/RTSGameplayTagLibrary.h"
#include "Libraries/RTSOrderLibrary.h"
#include "Orders/RTSAttackOrder.h"
#include "Orders/RTSBeginConstructionOrder.h"
#include "Orders/RTSContinueConstructionOrder.h"
#include "Orders/RTSGatherOrder.h"
#include "Orders/RTSMoveOrder.h"
#include "Orders/RTSReturnResourcesOrder.h"
#include "Abilities/RTSAbilitySystemComponent.h"
#include "Orders/RTSAttackMoveOrder.h"
#include "Orders/RTSStopOrder.h"


FPathFollowingRequestResult ARTSPawnAIController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
    FAIMoveRequest AdjustedRequest = MoveRequest;

    // When executing a gather order, use the gather range as the acceptance radius
    // so that pathfinding stops as soon as the unit is close enough to gather,
    // instead of trying to reach the resource center (which causes circling).
    if (HasOrderByClass(URTSGatherOrder::StaticClass()) && MoveRequest.IsMoveToActorRequest())
    {
        if (APawn* MyPawn = GetPawn())
        {
            if (URTSGathererComponent* GathererComp = MyPawn->FindComponentByClass<URTSGathererComponent>())
            {
                AActor* GoalActor = MoveRequest.GetGoalActor();
                if (IsValid(GoalActor))
                {
                    const float GatherRange = GathererComp->GetGatherRange(GoalActor);
                    if (GatherRange > AdjustedRequest.GetAcceptanceRadius())
                    {
                        AdjustedRequest.SetAcceptanceRadius(GatherRange);
                    }
                }
            }
        }
    }

    return Super::MoveTo(AdjustedRequest, OutPath);
}

void ARTSPawnAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

	AttackComponent = InPawn->FindComponentByClass<URTSAttackComponent>();

    // Make AI use assigned blackboard.
    UBlackboardComponent* BlackboardComponent;

	if (UseBlackboard(PawnBlackboardAsset, BlackboardComponent))
	{
		// Setup blackboard.
		IssueStopOrder();
	}

    // Run behavior tree.
    RunBehaviorTree(PawnBehaviorTreeAsset);
}

void ARTSPawnAIController::FindTargetInAcquisitionRadius()
{
    // Don't acquire targets while stunned — the BT's internal MoveTo bypasses the order system,
    // so we block at this level to prevent movement/attack under Immobilized.
    if (APawn* MyPawn = GetPawn())
    {
        if (URTSGameplayTagLibrary::HasGameplayTag(MyPawn, URTSGameplayTagLibrary::Status_Changing_Immobilized()))
        {
            Blackboard->ClearValue(TEXT("TargetActor"));
            return;
        }
    }

	if (!IsValid(AttackComponent))
	{
		UE_LOG(LogRTS, Warning, TEXT("[AttackMove] %s: AttackComponent is NULL, skipping target search."), GetPawn() ? *GetPawn()->GetName() : TEXT("?"));
		return;
	}

	const float AcqRadius = AttackComponent->GetAcquisitionRadius();

	// Find nearby actors.
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetPawn());

	TArray<AActor*> NearbyActors;
	UKismetSystemLibrary::SphereOverlapActors(this, GetPawn()->GetActorLocation(),
		AcqRadius, AcquisitionObjectTypes, APawn::StaticClass(), ActorsToIgnore,
		NearbyActors);

	const UClass* CurrentOrderClass = Blackboard->GetValueAsClass(TEXT("OrderClass"));
	UE_LOG(LogRTS, Log, TEXT("[AcquireTargets] %s (Order=%s): SphereOverlap r=%.0f found %d actors."),
		*GetPawn()->GetName(),
		CurrentOrderClass ? *CurrentOrderClass->GetName() : TEXT("None"),
		AcqRadius, NearbyActors.Num());

	// Find target to acquire.
	for (AActor* NearbyActor : NearbyActors)
	{
		if (!IsValid(NearbyActor))
		{
			continue;
		}

		// Check owner.
		const AActor* MyActor = GetPawn();

		if (IsValid(MyActor))
		{
			const URTSOwnerComponent* MyOwnerComponent = MyActor->FindComponentByClass<URTSOwnerComponent>();

			if (MyOwnerComponent && MyOwnerComponent->IsSameTeamAsActor(NearbyActor))
			{
				UE_LOG(LogRTS, Log, TEXT("[AcquireTargets] Skipping %s: same team."), *NearbyActor->GetName());
				continue;
			}
		}

		// Check if found attackable actor.
		if (!URTSGameplayTagLibrary::HasGameplayTag(NearbyActor, URTSGameplayTagLibrary::Status_Permanent_CanBeAttacked()))
		{
			UE_LOG(LogRTS, Log, TEXT("[AcquireTargets] Skipping %s: no CanBeAttacked tag."), *NearbyActor->GetName());
			continue;
		}

		// Skip dead actors (death animation may still be playing).
		if (!URTSGameplayTagLibrary::HasGameplayTag(NearbyActor, URTSGameplayTagLibrary::Status_Changing_Alive()))
		{
			UE_LOG(LogRTS, Log, TEXT("[AcquireTargets] Skipping %s: not alive."), *NearbyActor->GetName());
			continue;
		}

		// Acquire target.
		Blackboard->SetValueAsObject(TEXT("TargetActor"), NearbyActor);

		UE_LOG(LogRTS, Log, TEXT("%s automatically acquired target %s."), *GetPawn()->GetName(), *NearbyActor->GetName());
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("[AcquireTargets] %s: No target acquired."), *GetPawn()->GetName());

	// During attack-move, clear stale target so BT transitions correctly to the move branch.
	// For explicit attack orders, preserve the target set by ApplyOrder.
	if (Blackboard->GetValueAsClass(TEXT("OrderClass")) == URTSAttackMoveOrder::StaticClass())
	{
		Blackboard->ClearValue(TEXT("TargetActor"));
	}
}

TSubclassOf<URTSOrder> ARTSPawnAIController::GetCurrentOrder() const
{
	return Blackboard->GetValueAsClass(TEXT("OrderClass"));
}

bool ARTSPawnAIController::HasOrder(ERTSOrderType OrderType) const
{
    UE_LOG(LogRTS, Warning, TEXT("ARTSPawnAIController::HasOrder has been deprecated as of plugin version 1.2. Please use HasOrderByClass instead."));
    return Blackboard->GetValueAsEnum(TEXT("OrderType")) == static_cast<uint8>(OrderType);
}

bool ARTSPawnAIController::HasOrderByClass(TSubclassOf<URTSOrder> OrderClass) const
{
    return GetCurrentOrder() == OrderClass;
}

bool ARTSPawnAIController::IsIdle() const
{
    return HasOrderByClass(URTSStopOrder::StaticClass());
}

void ARTSPawnAIController::IssueOrder(const FRTSOrderData& Order)
{
    IssueOrder(Order, false);
}

void ARTSPawnAIController::IssueOrder(const FRTSOrderData& Order, bool bAppendToQueue)
{
    // When queuing, append the order if we are currently doing something.
    if (bAppendToQueue && Blackboard && !IsIdle())
    {
        OrderQueue.Add(Order);
        OnOrderQueueChanged.Broadcast(GetOwner(), OrderQueue);
        return;
    }

    if (!Blackboard)
    {
        UE_LOG(LogRTS, Warning, TEXT("Blackboard not set up for %s, can't receive orders. Check AI Controller Class and Auto Possess AI."), *GetPawn()->GetName());
        return;
    }

    // Non-queued orders cancel any pending ability and clear the queue.
    bHasPendingAbility = false;
    ClearOrderQueue();
    ApplyOrder(Order);
}

void ARTSPawnAIController::ApplyOrder(const FRTSOrderData& Order)
{
    // Unbind delegates from the previous order before updating CurrentOrder.
    UnbindOrderValidationDelegates();
    CurrentOrder = Order;
    BindOrderValidationDelegates(Order);

    // Update blackboard.
    ERTSOrderType OrderType = OrderClassToType(Order.OrderClass);

    // Project the order location onto the NavMesh so units can pathfind to points on elevated
    // objects (e.g. a cube above the NavMesh). Raw Order.TargetLocation is kept for visual
    // feedback; only the Blackboard value used by the BT for pathfinding is projected.
    FVector NavTargetLocation = Order.TargetLocation;
    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation ProjectedLocation;
        if (NavSys->ProjectPointToNavigation(Order.TargetLocation, ProjectedLocation, FVector(500.f, 500.f, 10000.f)))
        {
            NavTargetLocation = ProjectedLocation.Location;
        }
    }

    Blackboard->SetValueAsEnum(TEXT("OrderType"), static_cast<uint8>(OrderType));
    Blackboard->SetValueAsClass(TEXT("OrderClass"), Order.OrderClass);
    Blackboard->SetValueAsObject(TEXT("TargetActor"), Order.TargetActor);
    Blackboard->SetValueAsVector(TEXT("TargetLocation"), NavTargetLocation);
    Blackboard->SetValueAsInt(TEXT("BuildingClass"), Order.Index);

    if (OrderType == ERTSOrderType::ORDER_None)
    {
        Blackboard->SetValueAsVector(TEXT("HomeLocation"), GetPawn()->GetActorLocation());
    }
    else if (Order.OrderClass == URTSAttackMoveOrder::StaticClass())
    {
        // Chase radius is measured from the attack-move destination, not the spawn point.
        Blackboard->SetValueAsVector(TEXT("HomeLocation"), NavTargetLocation);
    }
    else
    {
        Blackboard->ClearValue(TEXT("HomeLocation"));
    }

    // Clean up any active gather state before interrupting with a new order.
    if (APawn* MyPawn = GetPawn())
    {
        if (URTSGathererComponent* GathererComp = MyPawn->FindComponentByClass<URTSGathererComponent>())
        {
            GathererComp->LeaveCurrentResourceSource();
        }
    }

    // Update behavior tree.
    UBehaviorTreeComponent* BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
    if (BehaviorTreeComponent)
    {
        BehaviorTreeComponent->RestartTree();
    }

    // Apply order logic.
    URTSOrderLibrary::IssueOrder(GetOwner(), Order);

    // Notify listeners.
    OnOrderChanged.Broadcast(GetOwner(), OrderType);
    OnCurrentOrderChanged.Broadcast(GetOwner(), Order);
}

void ARTSPawnAIController::SetPendingAbility(int32 AbilityIndex, AActor* TargetActor, const FVector& TargetLocation)
{
    bHasPendingAbility = true;
    PendingAbilityIndex = AbilityIndex;
    PendingAbilityTargetActor = TargetActor;
    PendingAbilityTargetLocation = TargetLocation;
    UE_LOG(LogRTS, Log, TEXT("[Ability Approach] %s: pending ability %d set, target=%s"),
        GetPawn() ? *GetPawn()->GetName() : TEXT("?"), AbilityIndex, *TargetLocation.ToString());
}

void ARTSPawnAIController::FinishCurrentOrder()
{
    UE_LOG(LogRTS, Log, TEXT("[Ability Approach] %s: FinishCurrentOrder called, bHasPendingAbility=%d"),
        GetPawn() ? *GetPawn()->GetName() : TEXT("?"), (int32)bHasPendingAbility);

    if (bHasPendingAbility)
    {
        bHasPendingAbility = false;
        if (APawn* MyPawn = GetPawn())
        {
            if (URTSAbilitySystemComponent* AbilitySystem = MyPawn->FindComponentByClass<URTSAbilitySystemComponent>())
            {
                UE_LOG(LogRTS, Log, TEXT("[Ability Approach] %s: firing ability %d via FinishCurrentOrder"),
                    *MyPawn->GetName(), PendingAbilityIndex);
                AbilitySystem->UseAbility(PendingAbilityIndex, PendingAbilityTargetActor.Get(), PendingAbilityTargetLocation);
            }
        }
        IssueStopOrder();
        return;
    }

    while (OrderQueue.Num() > 0)
    {
        FRTSOrderData NextOrder = OrderQueue[0];
        OrderQueue.RemoveAt(0);
        OnOrderQueueChanged.Broadcast(GetOwner(), OrderQueue);

        // Skip orders whose actor target was destroyed while waiting in the queue.
        if (NextOrder.TargetActor != nullptr && !IsValid(NextOrder.TargetActor))
        {
            continue;
        }

        ApplyOrder(NextOrder);
        return;
    }

    IssueStopOrder();
}

void ARTSPawnAIController::ClearOrderQueue()
{
    if (OrderQueue.Num() > 0)
    {
        OrderQueue.Empty();
        OnOrderQueueChanged.Broadcast(GetOwner(), OrderQueue);
    }
}

const TArray<FRTSOrderData>& ARTSPawnAIController::GetOrderQueue() const
{
    return OrderQueue;
}

void ARTSPawnAIController::ValidateCurrentOrder()
{
    if (!Blackboard || !GetPawn() || IsIdle())
    {
        return;
    }

    // Don't interrupt the order while the gatherer is returning resources —
    // removing the CarryingResources tag is the normal outcome of that action, not an external invalidation.
    if (URTSGathererComponent* GathererComp = GetPawn()->FindComponentByClass<URTSGathererComponent>())
    {
        if (GathererComp->IsReturningResources())
        {
            return;
        }
    }

    if (!URTSOrderLibrary::CanObeyOrder(CurrentOrder.OrderClass, GetPawn(), CurrentOrder.Index))
    {
        UE_LOG(LogRTS, Log, TEXT("%s: current order invalidated by tag change, advancing queue."), *GetPawn()->GetName());
        FinishCurrentOrder();
        return;
    }

    if (IsValid(CurrentOrder.TargetActor))
    {
        FRTSOrderTargetData TargetData = URTSOrderLibrary::GetOrderTargetData(
            GetPawn(), CurrentOrder.TargetActor, CurrentOrder.TargetLocation);
        if (!URTSOrderLibrary::IsValidOrderTarget(CurrentOrder.OrderClass, GetPawn(), TargetData, CurrentOrder.Index))
        {
            UE_LOG(LogRTS, Log, TEXT("%s: current order target invalidated by tag change, advancing queue."), *GetPawn()->GetName());
            FinishCurrentOrder();
        }
    }
}

void ARTSPawnAIController::BindOrderValidationDelegates(const FRTSOrderData& Order)
{
    APawn* Unit = GetPawn();
    if (!IsValid(Unit))
    {
        return;
    }

    URTSGameplayTagsComponent* UnitTagsComp = Unit->FindComponentByClass<URTSGameplayTagsComponent>();
    if (IsValid(UnitTagsComp))
    {
        UnitTagsComp->CurrentTagsChanged.AddDynamic(this, &ARTSPawnAIController::OnUnitTagsChanged);
    }

    if (IsValid(Order.TargetActor))
    {
        URTSGameplayTagsComponent* TargetTagsComp = Order.TargetActor->FindComponentByClass<URTSGameplayTagsComponent>();
        if (IsValid(TargetTagsComp))
        {
            TargetTagsComp->CurrentTagsChanged.AddDynamic(this, &ARTSPawnAIController::OnTargetTagsChanged);
        }
        Order.TargetActor->OnDestroyed.AddDynamic(this, &ARTSPawnAIController::OnTargetDestroyed);
    }
}

void ARTSPawnAIController::UnbindOrderValidationDelegates()
{
    APawn* Unit = GetPawn();
    if (IsValid(Unit))
    {
        URTSGameplayTagsComponent* UnitTagsComp = Unit->FindComponentByClass<URTSGameplayTagsComponent>();
        if (IsValid(UnitTagsComp))
        {
            UnitTagsComp->CurrentTagsChanged.RemoveDynamic(this, &ARTSPawnAIController::OnUnitTagsChanged);
        }
    }

    if (IsValid(CurrentOrder.TargetActor))
    {
        URTSGameplayTagsComponent* TargetTagsComp = CurrentOrder.TargetActor->FindComponentByClass<URTSGameplayTagsComponent>();
        if (IsValid(TargetTagsComp))
        {
            TargetTagsComp->CurrentTagsChanged.RemoveDynamic(this, &ARTSPawnAIController::OnTargetTagsChanged);
        }
        CurrentOrder.TargetActor->OnDestroyed.RemoveDynamic(this, &ARTSPawnAIController::OnTargetDestroyed);
    }
}

void ARTSPawnAIController::OnUnitTagsChanged(AActor* Actor, FGameplayTagContainer CurrentTags)
{
    ValidateCurrentOrder();
}

void ARTSPawnAIController::OnTargetTagsChanged(AActor* Actor, FGameplayTagContainer CurrentTags)
{
    ValidateCurrentOrder();
}

void ARTSPawnAIController::OnTargetDestroyed(AActor* DestroyedActor)
{
    if (CurrentOrder.TargetActor != DestroyedActor)
    {
        return;
    }

    // Unbind the target's delegates manually since it is being destroyed.
    URTSGameplayTagsComponent* TargetTagsComp = DestroyedActor->FindComponentByClass<URTSGameplayTagsComponent>();
    if (IsValid(TargetTagsComp))
    {
        TargetTagsComp->CurrentTagsChanged.RemoveDynamic(this, &ARTSPawnAIController::OnTargetTagsChanged);
    }
    DestroyedActor->OnDestroyed.RemoveDynamic(this, &ARTSPawnAIController::OnTargetDestroyed);
    CurrentOrder.TargetActor = nullptr;

    // Advance to the next queued order instead of stopping — preserves the player's queue.
    FinishCurrentOrder();
}

void ARTSPawnAIController::IssueAttackOrder(AActor* Target)
{
    if (!Blackboard) return;
    FRTSOrderData Order;
    Order.OrderClass = URTSAttackOrder::StaticClass();
    Order.TargetActor = Target;
    ApplyOrder(Order);
}

void ARTSPawnAIController::IssueBeginConstructionOrder(TSubclassOf<AActor> BuildingClass, const FVector& TargetLocation)
{
    if (!Blackboard) return;
    FRTSOrderData Order;
    Order.OrderClass = URTSBeginConstructionOrder::StaticClass();
    Order.Index = URTSConstructionLibrary::GetConstructableBuildingIndex(GetPawn(), BuildingClass);
    Order.TargetLocation = TargetLocation;
    ApplyOrder(Order);
}

void ARTSPawnAIController::IssueContinueConstructionOrder(AActor* ConstructionSite)
{
    if (!Blackboard) return;
    FRTSOrderData Order;
    Order.OrderClass = URTSContinueConstructionOrder::StaticClass();
    Order.TargetActor = ConstructionSite;
    ApplyOrder(Order);
}

void ARTSPawnAIController::IssueGatherOrder(AActor* ResourceSource)
{
    if (!Blackboard) return;
    FRTSOrderData Order;
    Order.OrderClass = URTSGatherOrder::StaticClass();
    Order.TargetActor = ResourceSource;
    ApplyOrder(Order);
}

void ARTSPawnAIController::IssueMoveOrder(const FVector& Location)
{
    FRTSOrderData Order;
    Order.OrderClass = URTSMoveOrder::StaticClass();
    Order.TargetLocation = Location;
    IssueOrder(Order);
}

void ARTSPawnAIController::IssueReturnResourcesOrder()
{
    // If the player has queued more orders after the current one, skip the
    // automatic return-resources phase and advance to the next queued order instead.
    if (OrderQueue.Num() > 0)
    {
        FinishCurrentOrder();
        return;
    }

	auto GathererComponent = GetPawn()->FindComponentByClass<URTSGathererComponent>();

	if (!GathererComponent)
	{
		return;
	}

	AActor* ResourceDrain = GathererComponent->FindClosestResourceDrain();

	if (!ResourceDrain)
	{
		return;
	}

    if (!Blackboard) return;
    FRTSOrderData Order;
    Order.OrderClass = URTSReturnResourcesOrder::StaticClass();
    Order.TargetActor = ResourceDrain;
    ApplyOrder(Order);
}

void ARTSPawnAIController::IssueStopOrder()
{
    UE_LOG(LogRTS, Log, TEXT("[Ability Approach] %s: IssueStopOrder called, bHasPendingAbility=%d"),
        GetPawn() ? *GetPawn()->GetName() : TEXT("?"), (int32)bHasPendingAbility);

    if (bHasPendingAbility)
    {
        bHasPendingAbility = false;
        if (APawn* MyPawn = GetPawn())
        {
            if (URTSAbilitySystemComponent* AbilitySystem = MyPawn->FindComponentByClass<URTSAbilitySystemComponent>())
            {
                UE_LOG(LogRTS, Log, TEXT("[Ability Approach] %s: firing ability %d via IssueStopOrder"),
                    *MyPawn->GetName(), PendingAbilityIndex);
                AbilitySystem->UseAbility(PendingAbilityIndex, PendingAbilityTargetActor.Get(), PendingAbilityTargetLocation);
            }
        }
    }

    FRTSOrderData Order;
    Order.OrderClass = URTSStopOrder::StaticClass();
    IssueOrder(Order);
}

ERTSOrderType ARTSPawnAIController::OrderClassToType(UClass* OrderClass) const
{
    if (OrderClass == URTSAttackOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_Attack;
    }
    else if (OrderClass == URTSAttackMoveOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_AttackMove;
    }
    else if (OrderClass == URTSBeginConstructionOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_BeginConstruction;
    }
    else if (OrderClass == URTSContinueConstructionOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_ContinueConstruction;
    }
    else if (OrderClass == URTSGatherOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_Gather;
    }
    else if (OrderClass == URTSMoveOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_Move;
    }
    else if (OrderClass == URTSReturnResourcesOrder::StaticClass())
    {
        return ERTSOrderType::ORDER_ReturnResources;
    }

    return ERTSOrderType::ORDER_None;
}
