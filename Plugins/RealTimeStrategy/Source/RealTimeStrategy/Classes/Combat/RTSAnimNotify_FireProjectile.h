#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "RTSAnimNotify_FireProjectile.generated.h"

/**
 * Place this notify in an attack montage at the frame when the projectile should be fired.
 * Calls URTSAttackComponent::FirePendingProjectile() on the owner (server-only).
 */
UCLASS(meta = (DisplayName = "RTS Fire Projectile"))
class REALTIMESTRATEGY_API URTSAnimNotify_FireProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
