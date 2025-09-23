// Project Maedan, all right incorporated.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MAERTSCameraComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MAEDAN_API UMAERTSCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMAERTSCameraComponent();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
							  FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float EdgeScrollBoundary = 50.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float EdgeScrollSpeed = 1000.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bEnableEdgeScrolling = true;

private:
	void HandleEdgeScrolling(float DeltaTime);
	APlayerController* GetPlayerController() const;		
};
