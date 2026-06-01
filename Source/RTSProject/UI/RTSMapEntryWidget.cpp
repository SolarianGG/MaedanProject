// Fill out your copyright notice in the Description page of Project Settings.

#include "RTSMapEntryWidget.h"

#include "Components/Button.h"

void URTSMapEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &URTSMapEntryWidget::HandleClicked);
	}
}

void URTSMapEntryWidget::NativeDestruct()
{
	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveDynamic(this, &URTSMapEntryWidget::HandleClicked);
	}

	Super::NativeDestruct();
}

void URTSMapEntryWidget::HandleClicked()
{
	OnClickedDelegate.Broadcast(this);
}
