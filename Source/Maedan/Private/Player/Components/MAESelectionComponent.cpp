// Project Maedan, all right incorporated.


#include "Components/MAESelectionComponent.h"

#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "MAESelectableInterface.h"

UMAESelectionComponent::UMAESelectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMAESelectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
										  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

AActor* UMAESelectionComponent::GetActorUnderCursor() const
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return nullptr;

	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		FVector Start = WorldLocation;
		FVector End = Start + WorldDirection * DeprojectLineLength; 

		DrawDebugLine(GetWorld(), Start, End, FColor::Red);
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.bTraceComplex = true;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, SelectionChannel, Params))
		{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 16.0f, FColor::Red, false, 3.0f);
			return HitResult.GetActor();
		}
	}

	return nullptr;
}

AActor* UMAESelectionComponent::SelectActorUnderCursor()
{
	AActor* ActorUnderCursor = GetActorUnderCursor();
    
	if (ActorUnderCursor && ActorUnderCursor->Implements<UMAESelectableInterface>())
	{
		CurrentlySelectedActors.Empty();
		CurrentlySelectedActors.Add(ActorUnderCursor);
        
		OnActorSelected.Broadcast(ActorUnderCursor);
		OnSelectionChanged.Broadcast();
        
		return ActorUnderCursor;
	}
    
	return nullptr;
}

void UMAESelectionComponent::StartSelectionRectangle(FVector2D ScreenPosition)
{
    CurrentSelectionRect.StartScreen = ScreenPosition;
    CurrentSelectionRect.EndScreen = ScreenPosition;
    CurrentSelectionRect.bIsSelecting = true;
}

void UMAESelectionComponent::UpdateSelectionRectangle(FVector2D ScreenPosition)
{
    if (CurrentSelectionRect.bIsSelecting)
    {
        CurrentSelectionRect.EndScreen = ScreenPosition;
    }
}

TArray<AActor*> UMAESelectionComponent::EndSelectionRectangle()
{
    TArray<AActor*> SelectedActors;
    
    if (!CurrentSelectionRect.bIsSelecting) return SelectedActors;

    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC) return SelectedActors;

    const FVector2D NormalizedStart = FVector2D(
        FMath::Min(CurrentSelectionRect.StartScreen.X, CurrentSelectionRect.EndScreen.X),
        FMath::Min(CurrentSelectionRect.StartScreen.Y, CurrentSelectionRect.EndScreen.Y)
    );
    
    const FVector2D NormalizedEnd = FVector2D(
        FMath::Max(CurrentSelectionRect.StartScreen.X, CurrentSelectionRect.EndScreen.X),
        FMath::Max(CurrentSelectionRect.StartScreen.Y, CurrentSelectionRect.EndScreen.Y)
    );

    for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        
        if (!Actor->Implements<UMAESelectableInterface>()) continue;

        FVector2D ScreenPosition{};
        if (PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPosition))
        {
            if (ScreenPosition.X >= NormalizedStart.X && ScreenPosition.X <= NormalizedEnd.X &&
                ScreenPosition.Y >= NormalizedStart.Y && ScreenPosition.Y <= NormalizedEnd.Y)
            {
                SelectedActors.Add(Actor);
            }
        }
    }

    CurrentlySelectedActors = SelectedActors;
    CurrentSelectionRect.bIsSelecting = false;
    OnSelectionChanged.Broadcast();

    return SelectedActors;
}

TArray<AActor*> UMAESelectionComponent::SelectActorsInRectangle(FVector2D StartScreen, FVector2D EndScreen)
{
    StartSelectionRectangle(StartScreen);
    UpdateSelectionRectangle(EndScreen);
    return EndSelectionRectangle();
}

