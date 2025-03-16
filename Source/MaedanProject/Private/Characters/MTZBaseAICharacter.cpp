// Maedan Project, All rights incorporated.


#include "Characters/MTZBaseAICharacter.h"
#include "Abilities/MTZAbilitySystemComponent.h"

AMTZBaseAICharacter::AMTZBaseAICharacter() : Super()
{
	AbilitySystemComponent = CreateDefaultSubobject<UMTZAbilitySystemComponent>("MTZAbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

void AMTZBaseAICharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}