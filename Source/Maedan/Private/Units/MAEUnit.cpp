// Project Maedan, all right incorporated.


#include "Units/MAEUnit.h"

#include "AIController.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

AMAEUnit::AMAEUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMAEUnit::SetupSelectionInput_Implementation(UEnhancedInputComponent* InputComp, APlayerController* PC)
{
	IMAESelectableInterface::SetupSelectionInput_Implementation(InputComp, PC);

	OwningPlayer = PC;
	InputComp->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &AMAEUnit::Move);
	UE_LOG(LogTemp, Warning, TEXT("Actor %s has been bind to the controller"), *GetName());
}

void AMAEUnit::OnSelected_Implementation(APlayerController* PC)
{
	IMAESelectableInterface::OnSelected_Implementation(PC);
	UE_LOG(LogTemp, Warning, TEXT("Actor %s has been selected"), *GetName());
}

void AMAEUnit::OnDeselected_Implementation(APlayerController* PC)
{
	IMAESelectableInterface::OnDeselected_Implementation(PC);
	UE_LOG(LogTemp, Warning, TEXT("Actor %s has been deselected"), *GetName());
}

void AMAEUnit::BeginPlay()
{
	Super::BeginPlay();
}

void AMAEUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMAEUnit::Move()
{
	UE_LOG(LogTemp, Warning, TEXT("Actor %s On Move"), *GetName());
	
	
	if (!OwningPlayer)
		return;

	FVector2D MousePosition;
	if (!OwningPlayer->GetMousePosition(MousePosition.X, MousePosition.Y))
		return;

	FVector WorldLocation, WorldDirection;
	if (!OwningPlayer->DeprojectScreenPositionToWorld(MousePosition.X, MousePosition.Y, WorldLocation, WorldDirection))
		return;

	FHitResult HitResult;
	FVector TraceEnd = WorldLocation + WorldDirection * 10000.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(this); 

	if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		DrawDebugSphere(GetWorld(), HitResult.Location, 25.0f, 12, FColor::Green, false, 2.0f);

		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(AICon, HitResult.Location);
		}
	}
}
