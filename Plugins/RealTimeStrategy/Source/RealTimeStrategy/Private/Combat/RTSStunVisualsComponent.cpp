#include "Combat/RTSStunVisualsComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "RTSLog.h"
#include "RTSGameplayTagsComponent.h"
#include "Libraries/RTSGameplayTagLibrary.h"


void URTSStunVisualsComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    URTSGameplayTagsComponent* TagsComp = Owner->FindComponentByClass<URTSGameplayTagsComponent>();
    if (IsValid(TagsComp))
    {
        TagsComp->CurrentTagsChanged.AddDynamic(this, &URTSStunVisualsComponent::OnTagsChanged);
    }
}

void URTSStunVisualsComponent::OnTagsChanged(AActor* Actor, FGameplayTagContainer CurrentTags)
{
    const bool bNowStunned = CurrentTags.HasTag(URTSGameplayTagLibrary::Status_Changing_Immobilized());

    UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: %s tags changed — bNowStunned=%d bWasStunned=%d"),
        *GetOwner()->GetName(), bNowStunned, bIsStunned);

    if (bNowStunned && !bIsStunned)
    {
        bIsStunned = true;
        StartStunVisuals();
    }
    else if (!bNowStunned && bIsStunned)
    {
        bIsStunned = false;
        StopStunVisuals();
    }
}

void URTSStunVisualsComponent::StartStunVisuals()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: StartStunVisuals on %s."), *Owner->GetName());

    // Play stun montage and loop it via OnMontageEnded until stun is removed.
    if (!IsValid(StunMontage))
    {
        UE_LOG(LogRTS, Warning, TEXT("URTSStunVisualsComponent: %s — StunMontage is not set!"), *Owner->GetName());
    }
    else
    {
        USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
        if (!IsValid(Mesh))
        {
            UE_LOG(LogRTS, Warning, TEXT("URTSStunVisualsComponent: %s — no SkeletalMeshComponent found, cannot play montage."), *Owner->GetName());
        }
        else
        {
            UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
            if (!IsValid(AnimInstance))
            {
                UE_LOG(LogRTS, Warning, TEXT("URTSStunVisualsComponent: %s — AnimInstance is null, cannot play montage."), *Owner->GetName());
            }
            else
            {
                CachedAnimInstance = AnimInstance;
                CachedAnimInstance->Montage_Play(StunMontage);

                // Loop the first section back to itself so the engine never blends out
                // between iterations — no idle flash between repeats.
                if (StunMontage->CompositeSections.Num() > 0)
                {
                    const FName SectionName = StunMontage->GetSectionName(0);
                    CachedAnimInstance->Montage_SetNextSection(SectionName, SectionName, StunMontage);
                }

                UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: %s — playing stun montage '%s' (native section loop)."),
                    *Owner->GetName(), *StunMontage->GetName());
            }
        }
    }

    // Spawn and attach Niagara effect.
    if (!IsValid(StunEffect))
    {
        UE_LOG(LogRTS, Warning, TEXT("URTSStunVisualsComponent: %s — StunEffect (Niagara) is not set!"), *Owner->GetName());
    }
    else
    {
        USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
        const bool bUseSocket = !StunEffectSocket.IsNone() && IsValid(Mesh) && Mesh->DoesSocketExist(StunEffectSocket);

        if (!bUseSocket && !StunEffectSocket.IsNone())
        {
            UE_LOG(LogRTS, Warning, TEXT("URTSStunVisualsComponent: %s — socket '%s' not found, attaching to root instead."),
                *Owner->GetName(), *StunEffectSocket.ToString());
        }

        if (bUseSocket)
        {
            // Snap to socket, then:
            // - offset is expressed in MESH local space (so (0,0,100) = up relative to mesh, regardless of socket orientation)
            // - rotation is expressed in SOCKET local space (so rotating the socket rotates the effect)
            ActiveStunEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
                StunEffect, Mesh, StunEffectSocket,
                FVector::ZeroVector, FRotator::ZeroRotator,
                EAttachLocation::SnapToTargetIncludingScale,
                /*bAutoDestroy=*/false);

            if (IsValid(ActiveStunEffect))
            {
                ActiveStunEffect->SetRelativeLocationAndRotation(StunEffectOffset, StunEffectRotation);
            }

            UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: %s — Niagara effect spawned at socket '%s' (offset=%s, rotation=%s)."),
                *Owner->GetName(), *StunEffectSocket.ToString(), *StunEffectOffset.ToString(), *StunEffectRotation.ToString());
        }
        else
        {
            ActiveStunEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
                StunEffect, Owner->GetRootComponent(), NAME_None,
                StunEffectOffset, StunEffectRotation,
                EAttachLocation::KeepRelativeOffset,
                /*bAutoDestroy=*/false);

            UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: %s — Niagara effect spawned at root (offset=%s, rotation=%s)."),
                *Owner->GetName(), *StunEffectOffset.ToString(), *StunEffectRotation.ToString());
        }
    }
}

void URTSStunVisualsComponent::StopStunVisuals()
{
    AActor* Owner = GetOwner();

    UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: StopStunVisuals on %s."),
        IsValid(Owner) ? *Owner->GetName() : TEXT("(destroyed)"));

    // Stop stun montage with a short blend-out.
    if (IsValid(CachedAnimInstance) && IsValid(StunMontage))
    {
        CachedAnimInstance->Montage_Stop(0.25f, StunMontage);
        UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: %s — stun montage stopped."),
            IsValid(Owner) ? *Owner->GetName() : TEXT("(destroyed)"));
    }
    CachedAnimInstance = nullptr;

    // Destroy Niagara effect.
    if (IsValid(ActiveStunEffect))
    {
        ActiveStunEffect->DestroyComponent();
        ActiveStunEffect = nullptr;
        UE_LOG(LogRTS, Log, TEXT("URTSStunVisualsComponent: %s — Niagara stun effect destroyed."),
            IsValid(Owner) ? *Owner->GetName() : TEXT("(destroyed)"));
    }
}
