#include "Combat/RTSDamageAreaActor.h"

#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "NiagaraComponent.h"

#include "RTSLog.h"
#include "RTSOwnerComponent.h"


ARTSDamageAreaActor::ARTSDamageAreaActor()
    : ReplicatedRadius(0.f)
    , CachedCaster(nullptr)
    , CachedInstigatorController(nullptr)
    , Radius(0.f)
    , DamagePerTick(0.f)
    , Duration(0.f)
    , TickInterval(1.f)
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = RootSceneComponent;

    GroundDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("GroundDecal"));
    GroundDecal->SetupAttachment(RootSceneComponent);
    GroundDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
    GroundDecal->SetVisibility(false);

    AreaVfxComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AreaVfx"));
    AreaVfxComponent->SetupAttachment(RootSceneComponent);
    AreaVfxComponent->bAutoActivate = false;

    AreaSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AreaSound"));
    AreaSoundComponent->SetupAttachment(RootSceneComponent);
    AreaSoundComponent->bAutoActivate = false;
}

void ARTSDamageAreaActor::Initialize(
    float InRadius,
    float InDuration,
    float InTickInterval,
    float InDamagePerTick,
    TSubclassOf<UDamageType> InDamageType,
    const TArray<TEnumAsByte<EObjectTypeQuery>>& InObjectTypeFilter,
    TSubclassOf<AActor> InClassFilter,
    AActor* InCaster)
{
    if (!HasAuthority())
    {
        return;
    }

    Radius = InRadius;
    Duration = InDuration;
    TickInterval = FMath::Max(0.05f, InTickInterval);
    DamagePerTick = InDamagePerTick;
    DamageType = InDamageType;
    ObjectTypeFilter = InObjectTypeFilter;
    ClassFilter = InClassFilter;
    CachedCaster = InCaster;

    if (APawn* CasterPawn = Cast<APawn>(InCaster))
    {
        CachedInstigatorController = CasterPawn->GetController();
    }

    ReplicatedRadius = InRadius;
    OnRep_VisualRadius();

    GetWorldTimerManager().SetTimer(
        TickTimer, this, &ARTSDamageAreaActor::OnDamageTick,
        TickInterval, /*bLoop*/ true, /*FirstDelay*/ TickInterval);

    if (Duration > 0.f)
    {
        GetWorldTimerManager().SetTimer(
            ExpiryTimer,
            [WeakSelf = TWeakObjectPtr<AActor>(this)]()
            {
                if (WeakSelf.IsValid())
                {
                    WeakSelf->Destroy();
                }
            },
            Duration, false);
    }
}

void ARTSDamageAreaActor::BeginPlay()
{
    Super::BeginPlay();

    if (AreaVfxComponent)
    {
        AreaVfxComponent->Activate();
    }

    if (IsValid(AreaLoopingSound) && AreaSoundComponent)
    {
        AreaSoundComponent->SetSound(AreaLoopingSound);
        AreaSoundComponent->Play();
    }
}

void ARTSDamageAreaActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSDamageAreaActor, ReplicatedRadius);
}

void ARTSDamageAreaActor::OnRep_VisualRadius()
{
    if (GroundDecal && ReplicatedRadius > 0.f)
    {
        GroundDecal->DecalSize = FVector(200.f, ReplicatedRadius, ReplicatedRadius);
        GroundDecal->SetVisibility(GroundDecal->GetDecalMaterial() != nullptr);
    }
}

void ARTSDamageAreaActor::OnDamageTick()
{
    if (!HasAuthority() || !IsValid(CachedCaster))
    {
        return;
    }

    TArray<AActor*> Overlapped;
    TArray<AActor*> ToIgnore;
    ToIgnore.Add(this);
    ToIgnore.Add(CachedCaster);

    UKismetSystemLibrary::CapsuleOverlapActors(
        this,
        GetActorLocation(),
        Radius,
        10000.f,
        ObjectTypeFilter,
        ClassFilter,
        ToIgnore,
        Overlapped);

    TSubclassOf<UDamageType> LocalDamageType = DamageType;

    for (AActor* Other : Overlapped)
    {
        if (!IsValid(Other))
        {
            continue;
        }

        URTSOwnerComponent* OwnerComp = Other->FindComponentByClass<URTSOwnerComponent>();
        if (!IsValid(OwnerComp))
        {
            continue;
        }

        if (OwnerComp->IsSameTeamAsActor(CachedCaster))
        {
            continue;
        }

        Other->TakeDamage(DamagePerTick, FDamageEvent(LocalDamageType), CachedInstigatorController, CachedCaster);
    }
}
