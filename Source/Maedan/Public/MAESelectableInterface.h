#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
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
	void OnSelected();
    
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS")
	void OnDeselected();
    
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS")
	bool IsSelectable() const;
};