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

    // Size the decal using the attached parent actor when available (attach happens before BeginPlay).
    AActor* Parent = GetAttachParentActor();
    if (IsValid(Parent))
    {
        float Radius = URTSCollisionLibrary::GetActorCollisionSize(Parent);
        float Height = URTSCollisionLibrary::GetActorCollisionHeight(Parent);
        DecalComponent->DecalSize = FVector(Height, Radius, Radius);
    }
    else
    {
        // Fallback: sphere overlap for cases where the circle is not attached.
        TArray<FOverlapResult> OverlapResults;
        FCollisionObjectQueryParams ObjectQueryParams;
        ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
        ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        if (GetWorld()->OverlapMultiByObjectType(OverlapResults, GetActorLocation(), FQuat::Identity,
            ObjectQueryParams, FCollisionShape::MakeSphere(500.0f), QueryParams))
        {
            if (OverlapResults[0].GetActor())
            {
                float Radius = URTSCollisionLibrary::GetActorCollisionSize(OverlapResults[0].GetActor());
                float Height = URTSCollisionLibrary::GetActorCollisionHeight(OverlapResults[0].GetActor());
                DecalComponent->DecalSize = FVector(Height, Radius, Radius);
            }
        }
    }

    DecalComponent->SetFadeOut(GetWorld()->GetTimeSeconds(), Duration, false);
    SetLifeSpan(Duration + 0.1f);
}
