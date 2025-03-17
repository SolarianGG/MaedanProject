// Maedan Project, All rights incorporated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/MTZAbilitySystemComponent.h"
#include "MTZBaseCharacter.generated.h"

UCLASS()
class MAEDANPROJECT_API AMTZBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMTZBaseCharacter();

protected:
	UPROPERTY()
	TObjectPtr<UMTZAbilitySystemComponent> AbilitySystemComponent{nullptr};
	virtual void BeginPlay() override;

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override final { return AbilitySystemComponent; }
	
};
