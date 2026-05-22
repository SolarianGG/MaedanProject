#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"

#include "GameFramework/DamageType.h"
#include "Templates/SubclassOf.h"

#include "Combat/RTSProjectile.h"

#include "RTSAttackData.generated.h"


USTRUCT(BlueprintType)
struct REALTIMESTRATEGY_API FRTSAttackData
{
	GENERATED_BODY()

public:
	/** Time before this attack can be used again, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float Cooldown;

	/** Damage dealt by this attack. Negative values could mean healing, depending on the game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float Damage;

	/** Type of the damage caused by this attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	TSubclassOf<UDamageType> DamageType;

	/** Range of this attack, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float Range;

	/** Anime montage that is executed on unit attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "RTS")
	UAnimMontage* AttackMontage = nullptr;

	/** Type of the projectile to spawn. If not set, damage will be dealt instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	TSubclassOf<ARTSProjectile> ProjectileClass;

	/** Optional socket name on the owner's skeletal mesh to spawn the projectile from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	FName ProjectileSpawnSocket;

	/** Delay in seconds after UseAttack before the projectile is spawned. Simple but drifts if animation speed changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	float ProjectileSpawnDelay = 0.0f;

	/** If true, the projectile is spawned when RTSAnimNotify_FireProjectile fires in the montage.
	 *  Frame-perfect sync regardless of animation speed. Requires the notify to be placed in AttackMontage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS")
	bool bWaitForAnimNotify = false;

	/** Maximum PlayRate for the attack montage. When the cooldown-derived PlayRate (MontageLength / Cooldown)
	 *  exceeds this value, the montage plays at MaxPlayRate and damage is applied immediately instead of via
	 *  AnimNotify, keeping damage correct while the animation looks natural. 0 = no cap (default behavior). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS", meta = (ClampMin = 0))
	float MaxPlayRate = 0.0f;
};
