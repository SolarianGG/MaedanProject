// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSMapEntryWidget.generated.h"

class UButton;
class URTSMapEntryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapEntryClicked, URTSMapEntryWidget*, Entry);

/**
 * Base class for a pre-authored map tile in the host-config map list. Designers subclass this
 * as a WBP (one per map), bake in MapPath/DisplayName + a preview image, and drop the instances
 * into the WrapBox bound as MapEntryContainer on URTSMenuWidget. Clicking broadcasts
 * OnClickedDelegate so the menu can track the selection.
 */
UCLASS()
class RTSPROJECT_API URTSMapEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Map travelled to for the match, e.g. "/Game/Maps/SKM-DarkSpace". Set in the WBP defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS|Map")
	FString MapPath;

	/** Optional display name for the map; shown by the WBP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SteamRTS|Map")
	FString DisplayName;

	/** Broadcast when this entry is clicked. The menu binds to this to update its selection. */
	FOnMapEntryClicked OnClickedDelegate;

	/** Implement the selected/unselected visuals (highlight frame, etc.) in the WBP. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SteamRTS|Map")
	void OnSelectionChanged(bool bSelected);

protected:
	/** Click source for selecting this entry; name it identically in the WBP. */
	UPROPERTY(BlueprintReadOnly, Transient, meta = (BindWidgetOptional))
	UButton* SelectButton;

private:
	UFUNCTION()
	void HandleClicked();
};
