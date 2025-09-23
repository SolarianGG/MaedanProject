// Project Maedan, all right incorporated.


#include "Player/Components/MAERTSCameraComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

UMAERTSCameraComponent::UMAERTSCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UMAERTSCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                       FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bEnableEdgeScrolling)
    {
        HandleEdgeScrolling(DeltaTime);
    }
}

void UMAERTSCameraComponent::HandleEdgeScrolling(float DeltaTime)
{
    APlayerController* PC = GetPlayerController();
    if (!PC) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    FVector2D MousePosition;
    FVector2D ViewportSize;
    
    if (!PC->GetMousePosition(MousePosition.X, MousePosition.Y)) return;
    
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    }

    if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return;

    FVector MovementDirection = FVector::ZeroVector;

    if (MousePosition.X <= EdgeScrollBoundary)
    {
        MovementDirection.Y = -1.0f; 
    }
    else if (MousePosition.X >= ViewportSize.X - EdgeScrollBoundary)
    {
        MovementDirection.Y = 1.0f;
    }

    if (MousePosition.Y <= EdgeScrollBoundary)
    {
        MovementDirection.X = 1.0f; 
    }
    else if (MousePosition.Y >= ViewportSize.Y - EdgeScrollBoundary)
    {
        MovementDirection.X = -1.0f; 
    }

    if (!MovementDirection.IsZero())
    {
        FVector WorldMovement = OwnerPawn->GetActorTransform().TransformVector(MovementDirection);
        WorldMovement.Z = 0;
        WorldMovement.Normalize();

        const FVector NewLocation = OwnerPawn->GetActorLocation() + 
                             WorldMovement * EdgeScrollSpeed * DeltaTime;
        OwnerPawn->SetActorLocation(NewLocation);
    }
}

APlayerController* UMAERTSCameraComponent::GetPlayerController() const
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}
