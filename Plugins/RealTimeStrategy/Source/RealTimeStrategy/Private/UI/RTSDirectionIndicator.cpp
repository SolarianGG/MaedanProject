#include "UI/RTSDirectionIndicator.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"


ARTSDirectionIndicator::ARTSDirectionIndicator()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ARTSDirectionIndicator::BeginPlay()
{
    Super::BeginPlay();

    // Create the decal component at runtime, same pattern as URTSSelectableComponent
    DecalComponent = NewObject<UDecalComponent>(this, TEXT("DirectionDecal"));
    DecalComponent->RegisterComponent();
    DecalComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

    // Pitch=-90 makes the decal project straight down
    DecalComponent->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, -90.0f, 0.0f)));
    DecalComponent->DecalSize = FVector(ProjectionDepth, 1.0f, 1.0f);

    if (LineMaterial)
    {
        UMaterialInstanceDynamic* MatInstance = UMaterialInstanceDynamic::Create(LineMaterial, this);
        DecalComponent->SetDecalMaterial(MatInstance);
    }

    DecalComponent->SetHiddenInGame(true);
}

void ARTSDirectionIndicator::SetTarget(FVector InStart, FVector InEnd)
{
    if (!IsValid(DecalComponent))
    {
        return;
    }

    FVector Direction(InEnd.X - InStart.X, InEnd.Y - InStart.Y, 0.0f);
    float Distance = Direction.Size();

    if (Distance < 1.0f)
    {
        return;
    }

    if (DecalComponent->bHiddenInGame)
    {
        DecalComponent->SetHiddenInGame(false);
    }

    SetActorLocation(FVector(
        (InStart.X + InEnd.X) * 0.5f,
        (InStart.Y + InEnd.Y) * 0.5f,
        InStart.Z + 10.0f
    ));

    FVector Dir2D = Direction / Distance;
    float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Dir2D.X, Dir2D.Y));
    SetActorRotation(FRotator(0.0f, Yaw, 0.0f));

    DecalComponent->DecalSize = FVector(ProjectionDepth, Distance * 0.5f, LineHalfWidth);
}
