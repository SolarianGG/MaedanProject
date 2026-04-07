#include "RTSTeamMaterialComponent.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Controller.h"

#include "RTSLog.h"
#include "RTSOwnerComponent.h"
#include "RTSPlayerState.h"
#include "RTSTeamInfo.h"


URTSTeamMaterialComponent::URTSTeamMaterialComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	MaterialSlotIndex = 0;
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

	UE_LOG(LogRTS, Log, TEXT("%s: RTSTeamMaterialComponent bound to OnOwnerChanged."), *Owner->GetName());

	// Apply material for the current owner if already set.
	ARTSPlayerState* PlayerOwner = OwnerComponent->GetPlayerOwner();
	if (IsValid(PlayerOwner))
	{
		ARTSTeamInfo* Team = PlayerOwner->GetTeam();
		if (IsValid(Team))
		{
			UE_LOG(LogRTS, Log, TEXT("%s: BeginPlay - owner already set, team %d."), *Owner->GetName(), Team->GetTeamIndex());
			ApplyTeamMaterial(Team->GetTeamIndex());
		}
		else
		{
			UE_LOG(LogRTS, Warning, TEXT("%s: BeginPlay - owner set but Team is null."), *Owner->GetName());
		}
	}
	else
	{
		UE_LOG(LogRTS, Log, TEXT("%s: BeginPlay - no owner yet, waiting for OnOwnerChanged."), *Owner->GetName());
	}
}

void URTSTeamMaterialComponent::OnOwnerChanged(AActor* Actor, AController* NewOwner)
{
	UE_LOG(LogRTS, Log, TEXT("%s: OnOwnerChanged fired. NewOwner valid: %d"),
		Actor ? *Actor->GetName() : TEXT("null"), IsValid(NewOwner));

	if (!IsValid(NewOwner))
	{
		return;
	}

	ARTSPlayerState* PlayerState = Cast<ARTSPlayerState>(NewOwner->PlayerState);
	if (!IsValid(PlayerState))
	{
		UE_LOG(LogRTS, Warning, TEXT("%s: OnOwnerChanged - PlayerState is null."), *Actor->GetName());
		return;
	}

	ARTSTeamInfo* Team = PlayerState->GetTeam();
	if (!IsValid(Team))
	{
		UE_LOG(LogRTS, Warning, TEXT("%s: OnOwnerChanged - Team is null."), *Actor->GetName());
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("%s: OnOwnerChanged - applying material for team %d."), *Actor->GetName(), Team->GetTeamIndex());
	ApplyTeamMaterial(Team->GetTeamIndex());
}

void URTSTeamMaterialComponent::ApplyTeamMaterial(uint8 TeamIndex)
{
	UE_LOG(LogRTS, Log, TEXT("ApplyTeamMaterial called: TeamIndex=%d, TeamMaterials.Num=%d"), TeamIndex, TeamMaterials.Num());

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

	UMeshComponent* MeshComponent = Owner->FindComponentByClass<UMeshComponent>();
	if (!IsValid(MeshComponent))
	{
		UE_LOG(LogRTS, Warning, TEXT("%s has RTSTeamMaterialComponent but no MeshComponent."), *Owner->GetName());
		return;
	}

	UE_LOG(LogRTS, Log, TEXT("%s: Setting material slot %d to %s on %s."),
		*Owner->GetName(), MaterialSlotIndex, *TeamMaterials[TeamIndex]->GetName(), *MeshComponent->GetName());
	MeshComponent->SetMaterial(MaterialSlotIndex, TeamMaterials[TeamIndex]);
}
