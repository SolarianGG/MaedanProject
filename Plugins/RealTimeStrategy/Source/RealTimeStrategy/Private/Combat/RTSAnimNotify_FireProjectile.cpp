#include "Combat/RTSAnimNotify_FireProjectile.h"

#include "Components/SkeletalMeshComponent.h"

#include "Combat/RTSAttackComponent.h"


void URTSAnimNotify_FireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	URTSAttackComponent* AttackComp = Owner->FindComponentByClass<URTSAttackComponent>();
	if (AttackComp)
	{
		AttackComp->FirePendingProjectile();
	}
}
