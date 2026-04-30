#include "Abilities/RTSDoTAreaAbility.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#include "RTSLog.h"
#include "Abilities/RTSAbilityTargetType.h"
#include "Combat/RTSDamageAreaActor.h"
#include "Orders/RTSOrderTargetData.h"


bool URTSDoTAreaAbility::IsValidAbilityTarget(const AActor* Caster, const FRTSOrderTargetData& TargetData) const
{
    if (!Super::IsValidAbilityTarget(Caster, TargetData))
    {
        return false;
    }

    if (TargetType != ERTSAbilityTargetType::ABILITYTARGET_Location)
    {
        return false;
    }

    if (!IsValid(Caster))
    {
        return false;
    }

    if (Range > 0.f)
    {
        const float Distance = FVector::Dist2D(Caster->GetActorLocation(), TargetData.Location);
        if (Distance > Range)
        {
            return false;
        }
    }

    return true;
}

void URTSDoTAreaAbility::ActivateAbility(AActor* Caster, const FRTSOrderTargetData& TargetData) const
{
    if (!IsValid(Caster) || !Caster->HasAuthority())
    {
        return;
    }

    TSubclassOf<ARTSDamageAreaActor> LocalAreaActorClass = AreaActorClass;
    if (!*LocalAreaActorClass)
    {
        UE_LOG(LogRTS, Warning, TEXT("URTSDoTAreaAbility %s: AreaActorClass is not set."), *GetName());
        return;
    }

    UWorld* World = Caster->GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.Instigator = Cast<APawn>(Caster);
    SpawnInfo.Owner = Caster;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnInfo.ObjectFlags |= RF_Transient;

    const FTransform Xform(FRotator::ZeroRotator, TargetData.Location);

    ARTSDamageAreaActor* Zone = World->SpawnActor<ARTSDamageAreaActor>(LocalAreaActorClass, Xform, SpawnInfo);
    if (!IsValid(Zone))
    {
        UE_LOG(LogRTS, Warning, TEXT("URTSDoTAreaAbility %s: failed to spawn zone actor."), *GetName());
        return;
    }

    Zone->Initialize(
        DoTRadius,
        DoTDuration,
        DoTTickInterval,
        DoTDamagePerTick,
        DamageType,
        AreaObjectTypeFilter,
        AreaClassFilter,
        Caster);

    UE_LOG(LogRTS, Log, TEXT("Actor %s spawned DoT area %s at %s (Radius=%.1f Duration=%.1f Tick=%.1f Dmg=%.1f)."),
        *Caster->GetName(), *Zone->GetName(), *TargetData.Location.ToCompactString(),
        DoTRadius, DoTDuration, DoTTickInterval, DoTDamagePerTick);
}
