// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MAESelectableInterface.h"
#include "MAESelectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorSelected, AActor*, SelectedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSelectionChanged);

USTRUCT(BlueprintType)
struct FSelectionRectangle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector2D StartScreen = FVector2D::Zero();
    
	UPROPERTY(BlueprintReadWrite)
	FVector2D EndScreen = FVector2D::Zero();
    
	UPROPERTY(BlueprintReadWrite)
	bool bIsSelecting = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MAEDAN_API UMAESelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMAESelectionComponent();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
							  FActorComponentTickFunction* ThisTickFunction) override;

	AActor* GetActorUnderCursor() const;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	TEnumAsByte<ECollisionChannel> SelectionChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	float DeprojectLineLength = 100000.f;

public:
	UFUNCTION(BlueprintCallable, Category = "Selection")
	AActor* SelectActorUnderCursor();

	UFUNCTION(BlueprintCallable, Category = "Selection")
	TArray<AActor*> SelectActorsInRectangle(FVector2D StartScreen, FVector2D EndScreen);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void StartSelectionRectangle(FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void UpdateSelectionRectangle(FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	TArray<AActor*> EndSelectionRectangle();

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnActorSelected OnActorSelected;
    
	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnSelectionChanged OnSelectionChanged;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Selection")
	TArray<AActor*> CurrentlySelectedActors;

	UPROPERTY(BlueprintReadWrite, Category = "Selection")
	FSelectionRectangle CurrentSelectionRect;
};
