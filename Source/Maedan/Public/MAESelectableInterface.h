#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "MAESelectableInterface.generated.h"


UINTERFACE(MinimalAPI)
class UMAESelectableInterface : public UInterface
{
	GENERATED_BODY()
};

class MAEDAN_API IMAESelectableInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS")
	UInputMappingContext* GetInputMapping() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS")
	void SetupSelectionInput(UEnhancedInputComponent* InputComp, APlayerController* PC);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS")
	void OnSelected(APlayerController* PC);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS")
	void OnDeselected(APlayerController* PC);
};
