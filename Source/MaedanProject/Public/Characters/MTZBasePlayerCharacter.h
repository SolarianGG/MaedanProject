// Maedan Project, All rights incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Characters/MTZBaseCharacter.h"
#include "MTZBasePlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class MAEDANPROJECT_API AMTZBasePlayerCharacter : public AMTZBaseCharacter
{
	GENERATED_BODY()

public:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
};
