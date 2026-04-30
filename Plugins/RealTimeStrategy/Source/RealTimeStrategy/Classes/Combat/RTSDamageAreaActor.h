#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Engine/EngineTypes.h"
#include "Templates/SubclassOf.h"

#include "RTSDamageAreaActor.generated.h"


class UDecalComponent;
class UNiagaraComponent;
class USceneComponent;


/**
 * Persistent area-of-effect actor that periodically deals damage to enemies of its caster.
 * Spawned by URTSDoTAreaAbility on the server; replicates to clients for VFX and decal display.
 */
UCLASS()
class REALTIMESTRATEGY_API ARTSDamageAreaActor : public AActor
{
    GENERATED_BODY()

public:
    ARTSDamageAreaActor();

    /** Configures the zone and starts damage ticks. Server-only. */
    void Initialize(
        float InRadius,
        float InDuration,
        float InTickInterval,
        float InDamagePerTick,
        TSubclassOf<UDamageType> InDamageType,
        const TArray<TEnumAsByte<EObjectTypeQuery>>& InObjectTypeFilter,
        TSubclassOf<AActor> InClassFilter,
        AActor* InCaster);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    UDecalComponent* GroundDecal;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS")
    UNiagaraComponent* AreaVfxComponent;

private:
    void OnDamageTick();

    UFUNCTION()
    void OnRep_VisualRadius();

    UPROPERTY(ReplicatedUsing = OnRep_VisualRadius)
    float ReplicatedRadius;

    UPROPERTY()
    AActor* CachedCaster;

    UPROPERTY()
    AController* CachedInstigatorController;

    TSubclassOf<UDamageType> DamageType;

    float Radius;
    float DamagePerTick;
    float Duration;
    float TickInterval;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypeFilter;
    TSubclassOf<AActor> ClassFilter;

    FTimerHandle TickTimer;
    FTimerHandle ExpiryTimer;
};
