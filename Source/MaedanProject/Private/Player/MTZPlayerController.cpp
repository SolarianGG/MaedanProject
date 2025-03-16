// Maedan Project, All rights incorporated.


#include "Player/MTZPlayerController.h"

#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"

void AMTZPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (auto* LocalPlayer{ Cast<ULocalPlayer>(Player) })
	{
		if (auto* InputSystem{ LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() })
		{
			if (InputMappingContext.IsValid())
			{
				InputSystem->AddMappingContext(InputMappingContext.LoadSynchronous(), 0);
			}
		}
	}
}	