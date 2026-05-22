#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RTSAnimNotify_MeleeAttack.generated.h"

/**
 * Place this notify in a melee attack montage at the hit frame.
 * Calls URTSAttackComponent::ApplyPendingMeleeAttack() on the owner (server-only).
 * Requires FRTSAttackData::bWaitForAnimNotify = true and ProjectileClass = null.
 */
UCLASS(meta = (DisplayName = "RTS Apply Melee Damage"))
class REALTIMESTRATEGY_API URTSAnimNotify_MeleeAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
