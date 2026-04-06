#include "RTSPawnAIController.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
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
	if (!IsValid(AttackComponent))
	{
		return;
	}

	// Find nearby actors.
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetPawn());
	
	TArray<AActor*> NearbyActors;
	UKismetSystemLibrary::SphereOverlapActors(this, GetPawn()->GetActorLocation(),
		AttackComponent->GetAcquisitionRadius(), AcquisitionObjectTypes, APawn::StaticClass(), ActorsToIgnore,
		NearbyActors);
	
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
				continue;
			}
		}

		// Check if found attackable actor.
		if (!URTSGameplayTagLibrary::HasGameplayTag(NearbyActor, URTSGameplayTagLibrary::Status_Permanent_CanBeAttacked()))
		{
			continue;
		}

		// Acquire target.
		Blackboard->SetValueAsObject(TEXT("TargetActor"), NearbyActor);

		UE_LOG(LogRTS, Log, TEXT("%s automatically acquired target %s."), *GetPawn()->GetName(), *NearbyActor->GetName());
		return;
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

    // Non-queued orders clear any pending queue.
    ClearOrderQueue();

    // Unbind delegates from the previous order before updating CurrentOrder.
    UnbindOrderValidationDelegates();
    CurrentOrder = Order;
    BindOrderValidationDelegates(Order);

    // Update blackboard.
    ERTSOrderType OrderType = OrderClassToType(Order.OrderClass);

    Blackboard->SetValueAsEnum(TEXT("OrderType"), static_cast<uint8>(OrderType));
    Blackboard->SetValueAsClass(TEXT("OrderClass"), Order.OrderClass);
    Blackboard->SetValueAsObject(TEXT("TargetActor"), Order.TargetActor);
    Blackboard->SetValueAsVector(TEXT("TargetLocation"), Order.TargetLocation);
    Blackboard->SetValueAsInt(TEXT("BuildingClass"), Order.Index);

    if (OrderType == ERTSOrderType::ORDER_None)
    {
        Blackboard->SetValueAsVector(TEXT("HomeLocation"), GetPawn()->GetActorLocation());
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

void ARTSPawnAIController::FinishCurrentOrder()
{
    if (OrderQueue.Num() > 0)
    {
        FRTSOrderData NextOrder = OrderQueue[0];
        OrderQueue.RemoveAt(0);
        OnOrderQueueChanged.Broadcast(GetOwner(), OrderQueue);
        IssueOrder(NextOrder, false);
    }
    else
    {
        IssueStopOrder();
    }
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
        UE_LOG(LogRTS, Log, TEXT("%s: current order invalidated by tag change, stopping."), *GetPawn()->GetName());
        IssueStopOrder();
        return;
    }

    if (IsValid(CurrentOrder.TargetActor))
    {
        FRTSOrderTargetData TargetData = URTSOrderLibrary::GetOrderTargetData(
            GetPawn(), CurrentOrder.TargetActor, CurrentOrder.TargetLocation);
        if (!URTSOrderLibrary::IsValidOrderTarget(CurrentOrder.OrderClass, GetPawn(), TargetData, CurrentOrder.Index))
        {
            UE_LOG(LogRTS, Log, TEXT("%s: current order target invalidated by tag change, stopping."), *GetPawn()->GetName());
            IssueStopOrder();
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

    IssueStopOrder();
}

void ARTSPawnAIController::IssueAttackOrder(AActor* Target)
{
    FRTSOrderData Order;
    Order.OrderClass = URTSAttackOrder::StaticClass();
    Order.TargetActor = Target;

    IssueOrder(Order);
}

void ARTSPawnAIController::IssueBeginConstructionOrder(TSubclassOf<AActor> BuildingClass, const FVector& TargetLocation)
{
    FRTSOrderData Order;
    Order.OrderClass = URTSBeginConstructionOrder::StaticClass();
    Order.Index = URTSConstructionLibrary::GetConstructableBuildingIndex(GetPawn(), BuildingClass);
    Order.TargetLocation = TargetLocation;

    IssueOrder(Order);
}

void ARTSPawnAIController::IssueContinueConstructionOrder(AActor* ConstructionSite)
{
    FRTSOrderData Order;
    Order.OrderClass = URTSContinueConstructionOrder::StaticClass();
    Order.TargetActor = ConstructionSite;
    IssueOrder(Order);
}

void ARTSPawnAIController::IssueGatherOrder(AActor* ResourceSource)
{
    FRTSOrderData Order;
    Order.OrderClass = URTSGatherOrder::StaticClass();
    Order.TargetActor = ResourceSource;
    IssueOrder(Order);
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

    FRTSOrderData Order;
    Order.OrderClass = URTSReturnResourcesOrder::StaticClass();
    Order.TargetActor = ResourceDrain;
    IssueOrder(Order);
}

void ARTSPawnAIController::IssueStopOrder()
{
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
