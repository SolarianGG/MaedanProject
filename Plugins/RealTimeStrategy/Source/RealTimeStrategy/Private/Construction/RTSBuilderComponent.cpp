#include "Construction/RTSBuilderComponent.h"

#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

#include "RTSPawnAIController.h"
#include "RTSPlayerController.h"
#include "RTSContainerComponent.h"
#include "RTSGameMode.h"
#include "RTSGameplayTagsComponent.h"
#include "RTSLog.h"
#include "Construction/RTSConstructionSiteComponent.h"
#include "Economy/RTSPlayerResourcesComponent.h"
#include "Libraries/RTSCollisionLibrary.h"
#include "Libraries/RTSGameplayLibrary.h"
#include "Components/ShapeComponent.h"
#include "Engine/World.h"
#include "RTSOwnerComponent.h"
#include "Libraries/RTSGameplayTagLibrary.h"


URTSBuilderComponent::URTSBuilderComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
    : Super(ObjectInitializer)
{
    // Set reasonable default values.
    InitialGameplayTags.AddTag(URTSGameplayTagLibrary::Status_Permanent_CanConstruct());
}

void URTSBuilderComponent::AssignToConstructionSite(AActor* ConstructionSite)
{
	if (!ConstructionSite)
	{
		return;
	}

	if (AssignedConstructionSite == ConstructionSite)
	{
		return;
	}

	auto ConstructionSiteComponent = ConstructionSite->FindComponentByClass<URTSConstructionSiteComponent>();

	if (!ConstructionSiteComponent)
	{
		return;
	}

	if (ConstructionSiteComponent->CanAssignBuilder(GetOwner()))
	{
		// Assign builder.
		AssignedConstructionSite = ConstructionSite;
		ConstructionSiteComponent->AssignBuilder(GetOwner());

		// Notify listeners.
		OnAssignedToConstructionSite.Broadcast(GetOwner(), ConstructionSite);

		UE_LOG(LogRTS, Log, TEXT("Builder %s assigned to construction site %s."), *GetOwner()->GetName(), *ConstructionSite->GetName());

		if (bEnterConstructionSite)
		{
            // Apply tags.
            URTSGameplayTagLibrary::AddGameplayTag(GetOwner(), URTSGameplayTagLibrary::Status_Changing_Constructing());

			// Enter construction site.
			auto ContainerComponent = ConstructionSite->FindComponentByClass<URTSContainerComponent>();

			if (ContainerComponent && 
                URTSGameplayTagLibrary::HasGameplayTag(ConstructionSite, URTSGameplayTagLibrary::Container_ConstructionSite()))
			{
				ContainerComponent->LoadActor(GetOwner());
				OnConstructionSiteEntered.Broadcast(GetOwner(), ConstructionSite);
			}
		}
	}
}

bool URTSBuilderComponent::BeginConstruction(TSubclassOf<AActor> BuildingClass, const FVector& TargetLocation)
{
	// Get game, pawn and controller.
	ARTSGameMode* GameMode = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));

	if (!GameMode)
	{
		return false;
	}

    auto* Pawn = Cast<APawn>(GetOwner());

    if (!Pawn)
    {
        return false;
    }

    auto* PawnController = Cast<ARTSPawnAIController>(Pawn->GetController());

    if (!PawnController)
    {
        return false;
    }

    if (BuildingClass == nullptr)
    {
        UE_LOG(LogRTS, Error, TEXT("Builder %s wants to build, but no building class was specified."), *GetOwner()->GetName());
        return false;
    }

    // Check requirements.
    TSubclassOf<AActor> MissingRequirement;

    if (URTSGameplayLibrary::GetMissingRequirementFor(this, GetOwner(), BuildingClass, MissingRequirement))
    {
        UE_LOG(LogRTS, Error, TEXT("Builder %s wants to build %s, but is missing requirement %s."), *GetOwner()->GetName(), *BuildingClass->GetName(), *MissingRequirement->GetName());

        auto* OwnerController = Cast<ARTSPlayerController>(GetOwner()->GetOwner());
        if (OwnerController)
        {
            OwnerController->NotifyOnErrorOccurred(
                FString::Printf(TEXT("Missing requirement: %s"), *MissingRequirement->GetName()));
        }

        NotifyOnConstructionFailed(Pawn, BuildingClass, Pawn->GetActorLocation());

        // Player is missing a required actor. Stop.
        PawnController->IssueStopOrder();
        return false;
    }

    // Move builder away in order to avoid collision.
    FVector BuilderLocation = Pawn->GetActorLocation();
    FVector ToTargetLocation = TargetLocation - BuilderLocation;
    ToTargetLocation.Z = 0.0f;
    FVector ToTargetLocationNormalized = ToTargetLocation.GetSafeNormal();
    float SafetyDistance =
        (URTSCollisionLibrary::GetActorCollisionSize(Pawn) / 2 +
         URTSCollisionLibrary::GetCollisionSize(BuildingClass) / 2)
        + ConstructionSiteOffset;

    FVector SafeBuilderLocation = TargetLocation - ToTargetLocationNormalized * SafetyDistance;
    SafeBuilderLocation.Z = BuilderLocation.Z;

    Pawn->SetActorLocation(SafeBuilderLocation);

    // Re-validate build location after builder has moved aside.
    // Friendly pawns are allowed (they will be pushed out); only enemy/neutral obstacles block construction.
    {
        UShapeComponent* BuildingShape =
            URTSGameplayLibrary::FindDefaultComponentByClass<UShapeComponent>(BuildingClass);

        if (BuildingShape)
        {
            FCollisionObjectQueryParams Params(FCollisionObjectQueryParams::AllDynamicObjects);
            TArray<FOverlapResult> Overlaps;
            GetWorld()->OverlapMultiByObjectType(
                Overlaps, TargetLocation, FQuat::Identity, Params, BuildingShape->GetCollisionShape());

            bool bLocationBlocked = false;
            for (const FOverlapResult& Overlap : Overlaps)
            {
                AActor* Actor = Overlap.Actor.Get();
                if (!Actor || Actor == GetOwner())
                {
                    continue;
                }

                // Friendly pawns are acceptable — they will be pushed aside below.
                if (Actor->IsA<APawn>())
                {
                    URTSOwnerComponent* OwnerComp = Actor->FindComponentByClass<URTSOwnerComponent>();
                    if (OwnerComp && OwnerComp->IsSameTeamAsActor(GetOwner()))
                    {
                        continue;
                    }
                }

                bLocationBlocked = true;
                break;
            }

            if (bLocationBlocked)
            {
                UE_LOG(LogRTS, Error, TEXT("Builder %s cannot place %s at %s - location is now obstructed."),
                    *GetOwner()->GetName(), *BuildingClass->GetName(), *TargetLocation.ToString());

                auto* OwnerController = Cast<ARTSPlayerController>(GetOwner()->GetOwner());
                if (OwnerController)
                {
                    OwnerController->NotifyOnErrorOccurred(TEXT("Build location is obstructed."));
                }

                NotifyOnConstructionFailed(Pawn, BuildingClass, TargetLocation);
                PawnController->IssueStopOrder();
                return false;
            }
        }
    }

    // Resource pre-check to prevent race condition (building flickering then vanishing).
    URTSConstructionSiteComponent* SiteComponent =
        URTSGameplayLibrary::FindDefaultComponentByClass<URTSConstructionSiteComponent>(BuildingClass);

    if (SiteComponent && SiteComponent->GetConstructionCostType() == ERTSPaymentType::PAYMENT_PayImmediately)
    {
        auto* OwnerController = Cast<ARTSPlayerController>(GetOwner()->GetOwner());
        if (OwnerController)
        {
            auto* PlayerResourcesComp = OwnerController->FindComponentByClass<URTSPlayerResourcesComponent>();
            if (PlayerResourcesComp && !PlayerResourcesComp->CanPayAllResources(SiteComponent->GetConstructionCosts()))
            {
                UE_LOG(LogRTS, Error, TEXT("Builder %s wants to build %s, but does not have enough resources."),
                    *GetOwner()->GetName(), *BuildingClass->GetName());
                NotifyOnConstructionFailed(Pawn, BuildingClass, Pawn->GetActorLocation());
                OwnerController->NotifyOnErrorOccurred(TEXT("Not enough resources."));
                PawnController->IssueStopOrder();
                return false;
            }
        }
    }

    // Push own-team units out of the building footprint before spawning.
    {
        UShapeComponent* BuildingShape =
            URTSGameplayLibrary::FindDefaultComponentByClass<UShapeComponent>(BuildingClass);

        if (BuildingShape)
        {
            FCollisionObjectQueryParams OverlapParams(FCollisionObjectQueryParams::AllDynamicObjects);
            TArray<FOverlapResult> Overlaps;
            GetWorld()->OverlapMultiByObjectType(
                Overlaps, TargetLocation, FQuat::Identity,
                OverlapParams, BuildingShape->GetCollisionShape());

            float BuildingRadius = URTSCollisionLibrary::GetCollisionSize(BuildingClass) / 2.0f;

            for (const FOverlapResult& Overlap : Overlaps)
            {
                AActor* Actor = Overlap.Actor.Get();
                if (!Actor || !Actor->IsA<APawn>() || Actor == GetOwner())
                {
                    continue;
                }

                URTSOwnerComponent* OwnerComp = Actor->FindComponentByClass<URTSOwnerComponent>();
                if (!OwnerComp || !OwnerComp->IsSameTeamAsActor(GetOwner()))
                {
                    continue; // don't push enemy units
                }

                ARTSPawnAIController* UnitController =
                    Cast<ARTSPawnAIController>(Cast<APawn>(Actor)->GetController());
                if (!UnitController)
                {
                    continue;
                }

                FVector ToUnit = Actor->GetActorLocation() - TargetLocation;
                ToUnit.Z = 0.0f;
                FVector ExitDirection = ToUnit.IsNearlyZero()
                    ? FVector(1.0f, 0.0f, 0.0f)
                    : ToUnit.GetSafeNormal();
                float UnitRadius = URTSCollisionLibrary::GetActorCollisionSize(Actor) / 2.0f;
                FVector ExitLocation = TargetLocation
                    + ExitDirection * (BuildingRadius + UnitRadius + 50.0f);
                ExitLocation.Z = Actor->GetActorLocation().Z;

                UnitController->IssueMoveOrder(ExitLocation);
            }
        }
    }

	// Spawn building.
	AActor* Building = GameMode->SpawnActorForPlayer(
		BuildingClass,
		Cast<AController>(GetOwner()->GetOwner()),
		FTransform(FRotator::ZeroRotator, TargetLocation));

	if (!Building)
	{
        NotifyOnConstructionFailed(Pawn, BuildingClass, Pawn->GetActorLocation());
		return false;
	}

	// Notify listeners.
	OnConstructionStarted.Broadcast(Pawn, Building);

	UE_LOG(LogRTS, Log, TEXT("Builder %s has created construction site %s."), *GetOwner()->GetName(), *Building->GetName());

	// Issue construction order.
	PawnController->IssueContinueConstructionOrder(Building);

    return true;
}

bool URTSBuilderComponent::BeginConstructionByIndex(int32 BuildingIndex, const FVector& TargetLocation)
{
	if (BuildingIndex < 0 || BuildingIndex >= ConstructibleBuildingClasses.Num())
	{
		return false;
	}

	return BeginConstruction(ConstructibleBuildingClasses[BuildingIndex], TargetLocation);
}

void URTSBuilderComponent::LeaveConstructionSite()
{
	if (!AssignedConstructionSite)
	{
		return;
	}

	auto ConstructionSite = AssignedConstructionSite;
	auto ConstructionSiteComponent = ConstructionSite->FindComponentByClass<URTSConstructionSiteComponent>();

	if (!ConstructionSiteComponent)
	{
		return;
	}

	// Remove builder.
	AssignedConstructionSite = nullptr;
	ConstructionSiteComponent->UnassignBuilder(GetOwner());

	// Notify listeners.
	OnRemovedFromConstructionSite.Broadcast(GetOwner(), ConstructionSite);

	UE_LOG(LogRTS, Log, TEXT("Builder %s has been unassigned from construction site %s."), *GetOwner()->GetName(), *ConstructionSite->GetName());

	if (bEnterConstructionSite)
	{
        // Remove tags.
        URTSGameplayTagLibrary::RemoveGameplayTag(GetOwner(), URTSGameplayTagLibrary::Status_Changing_Constructing());

		// Leave construction site.
		auto ContainerComponent = ConstructionSite->FindComponentByClass<URTSContainerComponent>();

		if (ContainerComponent)
		{
			ContainerComponent->UnloadActor(GetOwner());
			OnConstructionSiteLeft.Broadcast(GetOwner(), ConstructionSite);
		}
	}
}

TArray<TSubclassOf<AActor>> URTSBuilderComponent::GetConstructibleBuildingClasses() const
{
    return ConstructibleBuildingClasses;
}

bool URTSBuilderComponent::DoesEnterConstructionSite() const
{
    return bEnterConstructionSite;
}

float URTSBuilderComponent::GetConstructionSiteOffset() const
{
    return ConstructionSiteOffset;
}

AActor* URTSBuilderComponent::GetAssignedConstructionSite() const
{
    return AssignedConstructionSite;
}

TArray<USoundBase*> URTSBuilderComponent::GetConstructionFailedSounds() const
{
    return ConstructionFailedSounds;
}

void URTSBuilderComponent::NotifyOnConstructionFailed(AActor* Builder, TSubclassOf<AActor> BuildingClass, const FVector& Location)
{
    if (ConstructionFailedSounds.Num() > 0 && IsValid(Builder))
    {
        int32 Index = FMath::RandRange(0, ConstructionFailedSounds.Num() - 1);
        UGameplayStatics::PlaySoundAtLocation(this, ConstructionFailedSounds[Index], Builder->GetActorLocation());
    }

    OnConstructionFailed.Broadcast(Builder, BuildingClass, Location);
}
