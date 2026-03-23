#include "UI/RTSOrderTargetCircle.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "Libraries/RTSCollisionLibrary.h"


ARTSOrderTargetCircle::ARTSOrderTargetCircle()
{
    Duration = 0.5f;

    DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
    SetRootComponent(DecalComponent);
    DecalComponent->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, -90.0f, 0.0f)));
    DecalComponent->DecalSize = FVector(50.f, 50.f, 50.f);
}

void ARTSOrderTargetCircle::BeginPlay()
{
    Super::BeginPlay();

    if (CircleMaterial)
    {
        DecalComponent->SetDecalMaterial(CircleMaterial);
    }

    // Size the decal based on the collision of the actor at this location.
    TArray<AActor*> Overlapping;
    GetOverlappingActors(Overlapping);
    if (Overlapping.Num() > 0)
    {
        float Radius = URTSCollisionLibrary::GetActorCollisionSize(Overlapping[0]);
        float Height = URTSCollisionLibrary::GetActorCollisionHeight(Overlapping[0]);
        DecalComponent->DecalSize = FVector(Height, Radius, Radius);
    }

    // Fade out over Duration starting now. FadeStartDelay is absolute world time.
    DecalComponent->SetFadeOut(GetWorld()->GetTimeSeconds(), Duration, false);
    SetLifeSpan(Duration + 0.1f);
}
