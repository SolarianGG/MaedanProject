#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Materials/MaterialInterface.h"

#include "RTSTeamMaterialComponent.generated.h"


class ARTSPlayerState;
class ARTSTeamInfo;


/**
* Applies a team-specific material to the actor's mesh when ownership changes.
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class REALTIMESTRATEGY_API URTSTeamMaterialComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSTeamMaterialComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

private:
	/** Materials indexed by team index (element 0 = team 0, element 1 = team 1, etc.). */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	TArray<UMaterialInterface*> TeamMaterials;

	/** Which material slot on the mesh to replace. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	int32 MaterialSlotIndex;

	/** Component tag of the primary mesh to apply the material to. If empty, falls back to first SkeletalMesh/StaticMesh. */
	UPROPERTY(EditDefaultsOnly, Category = "RTS")
	FName PrimaryMeshComponentTag;

	UPROPERTY()
	ARTSPlayerState* PendingTeamPlayerState;

	UFUNCTION()
	void OnOwnerChanged(AActor* Actor, AController* NewOwner);

	UFUNCTION()
	void OnPlayerTeamChanged(ARTSTeamInfo* NewTeam);

	void SubscribeToTeamChanged(ARTSPlayerState* PlayerState);
	void UnsubscribeFromTeamChanged();

	void ApplyTeamMaterial(uint8 TeamIndex);
};
