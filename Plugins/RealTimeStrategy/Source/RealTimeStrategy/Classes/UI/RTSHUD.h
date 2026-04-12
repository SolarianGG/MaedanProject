#pragma once

#include "CoreMinimal.h"

#include "GameFramework/HUD.h"

#include "RTSHUD.generated.h"


class URTSProductionComponent;


/** Cached data about a drawn production queue icon for hit-testing. */
struct FRTSProductionQueueIconData
{
	FVector2D Position;
	FVector2D Size;
	AActor* ProductionActor;
	int32 QueueIndex;
	int32 ProductIndex;
};

/** Cached data about a drawn ability icon for hit-testing. */
struct FRTSAbilityIconData
{
	FVector2D Position;
	FVector2D Size;
	AActor* AbilityActor;
	int32 AbilityIndex;
};


/**
* HUD with RTS features, such as showing a selection frame.
*/
UCLASS()
class REALTIMESTRATEGY_API ARTSHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;

	/** Returns true if the given screen position is over a production queue icon. */
	bool IsPositionOnProductionQueue(float ScreenX, float ScreenY) const;

	/** Returns true if the given screen position is over an ability icon. */
	bool IsPositionOnAbilityIcons(float ScreenX, float ScreenY) const;

    /** Event for drawaing a floating combat text. */
    virtual void NotifyDrawFloatingCombatText(
        AActor* Actor,
        const FString& Text,
        const FLinearColor& Color,
        float Scale,
        float Lifetime,
        float RemainingLifetime,
        float LifetimePercentage,
        float SuggestedTextLeft,
        float SuggestedTextTop);

	/** Event for drawing the selection frame because the mouse is being dragged. */
	virtual void NotifyDrawSelectionFrame(float ScreenX, float ScreenY, float Width, float Height);

    /** Event for hiding the selection frame because the mouse isn't being dragged. */
    virtual void NotifyHideSelectionFrame();

    /** Event for drawaing a floating combat text. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS", meta = (DisplayName = "DrawFloatingCombatText"))
    void ReceiveDrawFloatingCombatText(
        AActor* Actor,
        const FString& Text,
        const FLinearColor& Color,
        float Scale,
        float Lifetime,
        float RemainingLifetime,
        float LifetimePercentage,
        float SuggestedTextLeft,
        float SuggestedTextTop);

	/** Event for drawing the selection frame because the mouse is being dragged. */
	UFUNCTION(BlueprintImplementableEvent, Category = "RTS", meta = (DisplayName = "DrawSelectionFrame"))
	void ReceiveDrawSelectionFrame(float ScreenX, float ScreenY, float Width, float Height);

    /** Event for hiding the selection frame because the mouse isn't being dragged. */
    UFUNCTION(BlueprintImplementableEvent, Category = "RTS", meta = (DisplayName = "HideSelectionFrame"))
    void ReceiveHideSelectionFrame();

	UFUNCTION(BlueprintPure)
	FVector2D GetActorCenterOnScreen(AActor* Actor) const;

	UFUNCTION(BlueprintPure)
	FVector2D GetActorSizeOnScreen(AActor* Actor) const;

private:
    /** Whether to always show all health bars. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Health Bars")
    bool bAlwaysShowHealthBars;

    /** Whether to show health bars for hovered units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Health Bars")
    bool bShowHoverHealthBars = true;

    /** Whether to show health bars for selected units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Health Bars")
    bool bShowSelectionHealthBars = true;

    /** Whether to show health bars while the respective hotkey is pressed. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Health Bars")
    bool bShowHotkeyHealthBars = true;


    /** Whether to always show all construction progress bars. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Construction Progress Bars")
    bool bAlwaysShowConstructionProgressBars;

    /** Whether to show construction progress bars for hovered units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Construction Progress Bars")
    bool bShowHoverConstructionProgressBars = true;

    /** Whether to show construction progress bars for selected units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Construction Progress Bars")
    bool bShowSelectionConstructionProgressBars = true;

    /** Whether to show construction progress bars while the respective hotkey is pressed. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Construction Progress Bars")
    bool bShowHotkeyConstructionProgressBars = true;


    /** Whether to always show all production progress bars. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Progress Bars")
    bool bAlwaysShowProductionProgressBars;

    /** Whether to show production progress bars for hovered units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Progress Bars")
    bool bShowHoverProductionProgressBars = true;

    /** Whether to show production progress bars for selected units. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Progress Bars")
    bool bShowSelectionProductionProgressBars = true;

    /** Whether to show production progress bars while the respective hotkey is pressed. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Progress Bars")
    bool bShowHotkeyProductionProgressBars = true;


    /** Whether to show floating combat texts above actors. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Floating Combat Texts")
    bool bShowFloatingCombatTexts = true;

    /** How many pixels the floating combat text should rise, per second. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Floating Combat Texts")
    float FloatingCombatTextSpeed = 20.0f;

    /** Whether to automatically adjust the alpha value of the color of floating combat texts depending on their elapsed lifetime. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Floating Combat Texts")
    bool bFadeOutFloatingCombatTexts = true;


    /** Whether we've been drawing the selection frame last frame, because the mouse was being dragged. */
    bool bWasDrawingSelectionFrame;

    /** Actor the player has hovering last frame. */
    UPROPERTY()
    AActor* OldHoveredActor;


	/** Draws the current selection frame if mouse is being dragged. */
	void DrawSelectionFrame();

    /** Draws floating combat texts. */
    void DrawFloatingCombatTexts();

	/** Draws unit health bars. */
	void DrawHealthBars();

	/** Draws the health bar of the specified actor. */
	void DrawHealthBar(AActor* Actor);

    /** Hides the health bar of the specified actor. */
    void HideHealthBar(AActor* Actor);

	/** Draws all construction progress bars. */
	void DrawConstructionProgressBars();

	/** Draws the construction progress bar of the specified actor. */
	void DrawConstructionProgressBar(AActor* Actor);

    /** Hides the construction progress bar of the specified actor. */
    void HideConstructionProgressBar(AActor* Actor);

	/** Draws a custom HUD effect for the currently hovered actor (e.g. player name). */
	void DrawHoveredActorWidget();

	/** Draws all production progress bars. */
	void DrawProductionProgressBars();

	/** Draws the production progress bar of the specified actor. */
	void DrawProductionProgressBar(AActor* Actor);

    /** Hides the production progress bar of the specified actor. */
    void HideProductionProgressBar(AActor* Actor);

	/** Draws the production queue icons for the selected building. */
	void DrawProductionQueue();

	/** Draws the ability icons for the selected unit. */
	void DrawAbilityIcons();

    /** Size of each production queue icon, in pixels. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Queue")
    float ProductionQueueIconSize = 64.0f;

    /** Padding between queue icons, in pixels. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Queue")
    float ProductionQueueIconPadding = 4.0f;

    /** Vertical offset from the bottom of the screen. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Production Queue")
    float ProductionQueueBottomOffset = 120.0f;

    /** Cached icon data for click detection. Rebuilt every frame. */
    TArray<FRTSProductionQueueIconData> ProductionQueueIcons;

    /** Size of each ability icon, in pixels. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Ability Icons")
    float AbilityIconSize = 64.0f;

    /** Padding between ability icons, in pixels. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Ability Icons")
    float AbilityIconPadding = 4.0f;

    /** Vertical offset from the bottom of the screen for ability icons. */
    UPROPERTY(EditDefaultsOnly, Category = "RTS|Ability Icons")
    float AbilityIconBottomOffset = 200.0f;

    /** Cached ability icon data for click detection. Rebuilt every frame. */
    TArray<FRTSAbilityIconData> AbilityIcons;
};
