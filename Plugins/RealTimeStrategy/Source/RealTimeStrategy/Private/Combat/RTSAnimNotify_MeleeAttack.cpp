#include "Combat/RTSAnimNotify_MeleeAttack.h"

#include "Components/SkeletalMeshComponent.h"

#include "Combat/RTSAttackComponent.h"


void URTSAnimNotify_MeleeAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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
		AttackComp->ApplyPendingMeleeAttack();
	}
}
