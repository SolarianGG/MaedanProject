#include "RTSTeamMaterialComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerState.h"
#include "RTSTeamInfo.h"


URTSTeamMaterialComponent::URTSTeamMaterialComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	MaterialSlotIndex = 0;
	PendingTeamPlayerState = nullptr;
}

void URTSTeamMaterialComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	URTSOwnerComponent* OwnerComponent = Owner->FindComponentByClass<URTSOwnerComponent>();
	if (!IsValid(OwnerComponent))
	{
		return;
	}

	OwnerComponent->OnOwnerChanged.AddDynamic(this, &URTSTeamMaterialComponent::OnOwnerChanged);

	// Apply material for the current owner if already set.
	ARTSPlayerState* PlayerOwner = OwnerComponent->GetPlayerOwner();
	if (IsValid(PlayerOwner))
	{
		ARTSTeamInfo* Team = PlayerOwner->GetTeam();
		if (IsValid(Team))
		{
			ApplyTeamMaterial(Team->GetTeamIndex());
		}
		else
		{
			// Team not yet replicated — wait for it.
			SubscribeToTeamChanged(PlayerOwner);
		}
	}
}

void URTSTeamMaterialComponent::OnOwnerChanged(AActor* Actor, AController* NewOwner)
{
	UnsubscribeFromTeamChanged();

	// AController* is nullptr on clients for non-local players — read PlayerOwner
	// directly from the OwnerComponent, which is replicated to all clients.
	if (!IsValid(Actor))
	{
		return;
	}

	URTSOwnerComponent* OwnerComponent = Actor->FindComponentByClass<URTSOwnerComponent>();
	if (!IsValid(OwnerComponent))
	{
		return;
	}

	ARTSPlayerState* PlayerState = OwnerComponent->GetPlayerOwner();
	if (!IsValid(PlayerState))
	{
		return;
	}

	ARTSTeamInfo* Team = PlayerState->GetTeam();
	if (IsValid(Team))
	{
		ApplyTeamMaterial(Team->GetTeamIndex());
	}
	else
	{
		// Team not yet replicated — wait for it.
		SubscribeToTeamChanged(PlayerState);
	}
}

void URTSTeamMaterialComponent::OnPlayerTeamChanged(ARTSTeamInfo* NewTeam)
{
	if (IsValid(NewTeam))
	{
		ApplyTeamMaterial(NewTeam->GetTeamIndex());
		UnsubscribeFromTeamChanged();
	}
}

void URTSTeamMaterialComponent::SubscribeToTeamChanged(ARTSPlayerState* PlayerState)
{
	UnsubscribeFromTeamChanged();

	PendingTeamPlayerState = PlayerState;
	if (IsValid(PendingTeamPlayerState))
	{
		PendingTeamPlayerState->OnTeamChangedDelegate.AddDynamic(this, &URTSTeamMaterialComponent::OnPlayerTeamChanged);
	}
}

void URTSTeamMaterialComponent::UnsubscribeFromTeamChanged()
{
	if (IsValid(PendingTeamPlayerState))
	{
		PendingTeamPlayerState->OnTeamChangedDelegate.RemoveDynamic(this, &URTSTeamMaterialComponent::OnPlayerTeamChanged);
		PendingTeamPlayerState = nullptr;
	}
}

void URTSTeamMaterialComponent::ApplyTeamMaterial(uint8 TeamIndex)
{
	if (!TeamMaterials.IsValidIndex(TeamIndex) || !TeamMaterials[TeamIndex])
	{
		UE_LOG(LogRTS, Warning, TEXT("ApplyTeamMaterial: invalid index %d or null material."), TeamIndex);
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// Find the primary mesh component using tag, then fallback to SkeletalMesh, StaticMesh, any Mesh.
	UMeshComponent* MeshComponent = nullptr;

	if (PrimaryMeshComponentTag.IsValid())
	{
		for (UActorComponent* Comp : Owner->GetComponentsByTag(UMeshComponent::StaticClass(), PrimaryMeshComponentTag))
		{
			MeshComponent = Cast<UMeshComponent>(Comp);
			break;
		}
	}

	if (!IsValid(MeshComponent))
	{
		MeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (!IsValid(MeshComponent))
	{
		MeshComponent = Owner->FindComponentByClass<UStaticMeshComponent>();
	}

	if (!IsValid(MeshComponent))
	{
		MeshComponent = Owner->FindComponentByClass<UMeshComponent>();
	}

	if (!IsValid(MeshComponent))
	{
		UE_LOG(LogRTS, Warning, TEXT("%s has RTSTeamMaterialComponent but no MeshComponent."), *Owner->GetName());
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("%s: Setting material slot %d to %s on %s."),
		*Owner->GetName(), MaterialSlotIndex, *TeamMaterials[TeamIndex]->GetName(), *MeshComponent->GetName());
	MeshComponent->SetMaterial(MaterialSlotIndex, TeamMaterials[TeamIndex]);
}
