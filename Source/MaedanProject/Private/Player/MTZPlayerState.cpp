// Maedan Project, All rights incorporated.


#include "Player/MTZPlayerState.h"

#include "AbilitySystemComponent.h"

AMTZPlayerState::AMTZPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UMTZAbilitySystemComponent>("MTZAbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);

	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);


	// TODO: Right now is not that important but net update frequency 100
	// can be very high for shipping game, so fix it before release
	SetNetUpdateFrequency(100.0f);
}