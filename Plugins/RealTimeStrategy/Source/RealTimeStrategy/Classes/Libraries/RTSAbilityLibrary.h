#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "Abilities/RTSAbilityData.h"
#include "Abilities/RTSAbilityTargetType.h"

#include "RTSAbilityLibrary.generated.h"


class AActor;
class URTSAbility;
class UTexture2D;


/**
* Utility functions for the ability system.
*/
UCLASS()
class REALTIMESTRATEGY_API URTSAbilityLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Whether the specified actor can use the ability at the given index. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static bool CanUseAbility(const AActor* Actor, int32 AbilityIndex);

    /** Gets the abilities of the specified actor. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static TArray<FRTSAbilityData> GetAbilities(const AActor* Actor);

    /** Gets the mana cost of the specified ability class. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static float GetAbilityManaCost(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the cooldown of the specified ability class, in seconds. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static float GetAbilityCooldown(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the range of the specified ability class, in cm. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static float GetAbilityRange(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the target type of the specified ability class. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static ERTSAbilityTargetType GetAbilityTargetType(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the icon of the specified ability class. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static UTexture2D* GetAbilityIcon(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the name of the specified ability class. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static FText GetAbilityName(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the description of the specified ability class. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static FText GetAbilityDescription(TSubclassOf<URTSAbility> AbilityClass);

    /** Gets the current mana of the specified actor. Returns 0 if no mana component found. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static float GetCurrentMana(const AActor* Actor);

    /** Gets the maximum mana of the specified actor. Returns 0 if no mana component found. */
    UFUNCTION(BlueprintPure, Category = "RTS")
    static float GetMaxMana(const AActor* Actor);
};
