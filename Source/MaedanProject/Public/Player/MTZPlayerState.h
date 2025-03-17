// Maedan Project, All rights incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Abilities/MTZAbilitySystemComponent.h"
#include "MTZPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class MAEDANPROJECT_API AMTZPlayerState final : public APlayerState, public IAbilitySystemInterface 
{
	GENERATED_BODY()

public:
	AMTZPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; };


protected:
	UPROPERTY()
	TObjectPtr<UMTZAbilitySystemComponent> AbilitySystemComponent{nullptr};
};
