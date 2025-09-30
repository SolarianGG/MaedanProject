// Project Maedan, all right incorporated.


#include "Units/MAEUnit.h"

AMAEUnit::AMAEUnit()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AMAEUnit::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMAEUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMAEUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

bool AMAEUnit::IsSelectable_Implementation() const
{
	return true;
}

void AMAEUnit::OnDeselected_Implementation()
{
}

void AMAEUnit::OnSelected_Implementation()
{
}

