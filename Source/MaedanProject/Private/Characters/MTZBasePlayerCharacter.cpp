// Maedan Project, All rights incorporated.


#include "Characters/MTZBasePlayerCharacter.h"

#include "MTZPlayerState.h"

void AMTZBasePlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (auto* PS { GetPlayerState<AMTZPlayerState>()})
	{
		AbilitySystemComponent = Cast<UMTZAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}


void AMTZBasePlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (auto* PS { GetPlayerState<AMTZPlayerState>()})
	{
		AbilitySystemComponent = Cast<UMTZAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	}
}