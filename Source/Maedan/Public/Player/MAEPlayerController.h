// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "GameFramework/PlayerController.h"
#include "MAEPlayerController.generated.h"

class UInputAction;
class UMAESelectionComponent;
class UInputMappingContext;
class AMAEGameMode;

// TODO: Move all of these to different file 
// Think about doing FCommandData differently
USTRUCT(BlueprintType)
struct FCommandData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Commands")
	FVector Location;
};

UCLASS()
class UMAECommand : public UObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(const FCommandData& Data){};
	virtual void Execute(AActor* Actor){};
};

class UMAEMoveCommand : public UMAECommand
{
public:
	virtual void Initialize(const FCommandData& Data) override;
	virtual void Execute(AActor* Actor) override;
private:
	FVector MovePosition{};
};

/**
 * 
 */
UCLASS()
class MAEDAN_API AMAEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMAEPlayerController();

protected:
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

	virtual void SetupInputComponent() override;


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Select")
	TObjectPtr<UMAESelectionComponent> SelectionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputMappingContext> CameraInputMappings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> SelectAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> MultipleSelection;

	// FOR TESTING, REMOVE LATER
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

private:
	TObjectPtr<AMAEGameMode> GameMode;
	
	FVector2D SelectionStartPosition;
	bool bIsSelecting = false;

	UFUNCTION(Server, Reliable)
	void ServerIssueCommands(const TArray<AActor*>& Actors, TSubclassOf<UMAECommand> Command, const FCommandData& Data);

	
	void OnSelectionStart();
	void OnSelectionUpdate();
	void OnSelectionEnd();
	void OnSelectActor();

	void OnMoveCommand();
};
