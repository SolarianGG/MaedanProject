// Maedan Project, All rights incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Characters/MTZBaseCharacter.h"
#include "MTZBaseAICharacter.generated.h"

/**
 * 
 */
UCLASS()
class MAEDANPROJECT_API AMTZBaseAICharacter : public AMTZBaseCharacter
{
	GENERATED_BODY()

public:
	AMTZBaseAICharacter();

	virtual void BeginPlay() override;
};
