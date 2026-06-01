#include "UI/RTSHUD.h"

#include "EngineUtils.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"

#include "RTSPlayerController.h"
#include "RTSPortraitComponent.h"
#include "Abilities/RTSAbility.h"
#include "Abilities/RTSAbilitySystemComponent.h"
#include "Combat/RTSHealthComponent.h"
#include "Combat/RTSHealthBarWidgetComponent.h"
#include "Combat/RTSManaComponent.h"
#include "Combat/RTSManaBarWidgetComponent.h"
#include "Upgrades/RTSPlayerUpgradeComponent.h"
#include "Construction/RTSConstructionSiteComponent.h"
#include "Construction/RTSConstructionProgressBarWidgetComponent.h"
#include "Libraries/RTSCollisionLibrary.h"
#include "Libraries/RTSGameplayLibrary.h"
#include "Production/RTSProductionComponent.h"
#include "Production/RTSProductionProgressBarWidgetComponent.h"
#include "UI/RTSFloatingCombatTextComponent.h"
#include "UI/RTSFloatingCombatTextData.h"
#include "UI/RTSHoveredActorWidgetComponent.h"
#include "Engine/UserInterfaceSettings.h"


void ARTSHUD::DrawHUD()
{
	Super::DrawHUD();

    DrawFloatingCombatTexts();
	DrawSelectionFrame();
	DrawHealthBars();
	DrawManaBars();
	DrawConstructionProgressBars();
	DrawProductionProgressBars();
	DrawHoveredActorWidget();
	//DrawProductionQueue();
	DrawAbilityIcons();
}

void ARTSHUD::NotifyDrawFloatingCombatText(AActor* Actor, const FString& Text, const FLinearColor& Color, float Scale, float Lifetime, float RemainingLifetime, float LifetimePercentage, float SuggestedTextLeft, float SuggestedTextTop)
{
    ReceiveDrawFloatingCombatText(
        Actor,
        Text,
        Color,
        Scale,
        Lifetime,
        RemainingLifetime,
        LifetimePercentage,
        SuggestedTextLeft,
        SuggestedTextTop);
}

void ARTSHUD::NotifyDrawSelectionFrame(float ScreenX, float ScreenY, float Width, float Height)
{
	ReceiveDrawSelectionFrame(ScreenX, ScreenY, Width, Height);
}

void ARTSHUD::NotifyHideSelectionFrame()
{
    ReceiveHideSelectionFrame();
}

FVector2D ARTSHUD::GetActorCenterOnScreen(AActor* Actor) const
{
	FVector ProjectedLocation = Project(Actor->GetActorLocation());
	return FVector2D(ProjectedLocation.X, ProjectedLocation.Y);
}

FVector2D ARTSHUD::GetActorSizeOnScreen(AActor* Actor) const
{
	// Get actor position projected on HUD.
    float ActorHeight = URTSCollisionLibrary::GetActorCollisionHeight(Actor);
    float ActorWidth = URTSCollisionLibrary::GetActorCollisionSize(Actor);

    FVector ActorTopPosition = Project(Actor->GetActorLocation() + (Actor->GetActorForwardVector() * ActorHeight));
    FVector ActorBottomPosition = Project(Actor->GetActorLocation() - (Actor->GetActorForwardVector() * ActorHeight));
    FVector ActorLeftPosition = Project(Actor->GetActorLocation() - (Actor->GetActorRightVector() * ActorWidth));
    FVector ActorRightPosition = Project(Actor->GetActorLocation() + (Actor->GetActorRightVector() * ActorWidth));

    float Width = FVector(ActorRightPosition - ActorLeftPosition).Size();
    float Height = FVector(ActorTopPosition - ActorBottomPosition).Size();

    return FVector2D(Width, Height);
}

void ARTSHUD::DrawSelectionFrame()
{
	// Get selection frame.
	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);

	if (!PlayerController)
	{
		return;
	}

	FIntRect SelectionFrame;

	if (!PlayerController->GetSelectionFrame(SelectionFrame))
	{
        if (bWasDrawingSelectionFrame)
        {
            NotifyHideSelectionFrame();
        }
        
        bWasDrawingSelectionFrame = false;
		return;
	}

	// Draw selection frame.
	NotifyDrawSelectionFrame(
		SelectionFrame.Min.X,
		SelectionFrame.Min.Y,
		SelectionFrame.Width(),
		SelectionFrame.Height());

    bWasDrawingSelectionFrame = true;
}

void ARTSHUD::DrawFloatingCombatTexts()
{
    if (!bShowFloatingCombatTexts)
    {
        return;
    }

    for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
    {
        AActor* Actor = *ActorIt;

        if (!IsValid(Actor))
        {
            return;
        }

        // Check floating combat texts.
        URTSFloatingCombatTextComponent* FloatingCombatTextComponent = Actor->FindComponentByClass<URTSFloatingCombatTextComponent>();

        if (!FloatingCombatTextComponent)
        {
            continue;
        }

        for (FRTSFloatingCombatTextData& TextData : FloatingCombatTextComponent->GetTexts())
        {
            // Calculate lifetime.
            float ElapsedLifetime = TextData.Lifetime - TextData.RemainingLifetime;
            float LifetimePercentage = ElapsedLifetime / TextData.Lifetime;

            // Calculate position.
            FVector2D Center = GetActorCenterOnScreen(Actor);
            FVector2D Size = GetActorSizeOnScreen(Actor);

            // Calculate color.
            FLinearColor TextColor = TextData.Color;

            if (bFadeOutFloatingCombatTexts)
            {
                TextColor.A = 1 - LifetimePercentage;
            }

            // Draw text.
            NotifyDrawFloatingCombatText(
                Actor,
                TextData.Text,
                TextColor,
                TextData.Scale,
                TextData.Lifetime,
                TextData.RemainingLifetime,
                LifetimePercentage,
                Center.X,
                Center.Y - (Size.Y / 2) - (FloatingCombatTextSpeed * ElapsedLifetime));
        }
    }
}

void ARTSHUD::DrawHealthBars()
{
	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);

	if (!PlayerController)
	{
		return;
	}

    for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
    {
        AActor* Actor = *ActorIt;
       
        if (!URTSGameplayLibrary::IsFullyVisibleForLocalClient(Actor))
        {
            HideHealthBar(Actor);
            continue;
        }

        // Check override conditions.
        if (bAlwaysShowHealthBars || (bShowHotkeyHealthBars && PlayerController->IsHealthBarHotkeyPressed()))
        {
            // Draw all health bars.
            DrawHealthBar(Actor);
        }
        else if (bShowHoverHealthBars && Actor == PlayerController->GetHoveredActor())
        {
            // Draw health bar for hovered actor.
            DrawHealthBar(Actor);
        }
        else if (bShowSelectionHealthBars && PlayerController->GetSelectedActors().Contains(Actor))
        {
            // Draw health bars for selected actors.
            DrawHealthBar(Actor);
        }
        else
        {
            HideHealthBar(Actor);
        }
    }
}

void ARTSHUD::DrawHealthBar(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// Check health.
	URTSHealthComponent* HealthComponent = Actor->FindComponentByClass<URTSHealthComponent>();

	if (!IsValid(HealthComponent))
	{
		return;
	}

	const float HealthPercentage = HealthComponent->GetCurrentHealth() / HealthComponent->GetMaximumHealth();

    // Draw health bar.
    URTSHealthBarWidgetComponent* HealthBarWidgetComponent = Actor->FindComponentByClass<URTSHealthBarWidgetComponent>();

    if (!IsValid(HealthBarWidgetComponent))
    {
        return;
    }

    FVector2D Size = GetActorSizeOnScreen(Actor);

    HealthBarWidgetComponent->UpdatePositionAndSize(Size);
    HealthBarWidgetComponent->SetVisibility(true);
}

void ARTSHUD::HideHealthBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    URTSHealthBarWidgetComponent* HealthBarWidgetComponent = Actor->FindComponentByClass<URTSHealthBarWidgetComponent>();

    if (!IsValid(HealthBarWidgetComponent))
    {
        return;
    }

    HealthBarWidgetComponent->SetVisibility(false);
}

void ARTSHUD::DrawManaBars()
{
    ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);

    if (!PlayerController)
    {
        return;
    }

    for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
    {
        AActor* Actor = *ActorIt;

        if (!URTSGameplayLibrary::IsFullyVisibleForLocalClient(Actor))
        {
            HideManaBar(Actor);
            continue;
        }

        // Check override conditions.
        if (bAlwaysShowManaBars || (bShowHotkeyManaBars && PlayerController->IsHealthBarHotkeyPressed()))
        {
            DrawManaBar(Actor);
        }
        else if (bShowHoverManaBars && Actor == PlayerController->GetHoveredActor())
        {
            DrawManaBar(Actor);
        }
        else if (bShowSelectionManaBars && PlayerController->GetSelectedActors().Contains(Actor))
        {
            DrawManaBar(Actor);
        }
        else
        {
            HideManaBar(Actor);
        }
    }
}

void ARTSHUD::DrawManaBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    URTSManaComponent* ManaComponent = Actor->FindComponentByClass<URTSManaComponent>();

    if (!IsValid(ManaComponent) || ManaComponent->GetMaximumMana() <= 0.0f)
    {
        return;
    }

    URTSManaBarWidgetComponent* ManaBarWidgetComponent = Actor->FindComponentByClass<URTSManaBarWidgetComponent>();

    if (!IsValid(ManaBarWidgetComponent))
    {
        return;
    }

    FVector2D Size = GetActorSizeOnScreen(Actor);

    ManaBarWidgetComponent->UpdatePositionAndSize(Size);
    ManaBarWidgetComponent->SetVisibility(true);
}

void ARTSHUD::HideManaBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    URTSManaBarWidgetComponent* ManaBarWidgetComponent = Actor->FindComponentByClass<URTSManaBarWidgetComponent>();

    if (!IsValid(ManaBarWidgetComponent))
    {
        return;
    }

    ManaBarWidgetComponent->SetVisibility(false);
}

void ARTSHUD::DrawConstructionProgressBars()
{
    ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);

    if (!PlayerController)
    {
        return;
    }

    for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
    {
        AActor* Actor = *ActorIt;

        if (!URTSGameplayLibrary::IsFullyVisibleForLocalClient(Actor))
        {
            HideConstructionProgressBar(Actor);
            continue;
        }

        // Check override conditions.
        if (bAlwaysShowConstructionProgressBars || (bShowHotkeyConstructionProgressBars && PlayerController->IsConstructionProgressBarHotkeyPressed()))
        {
            // Draw all progress bars.
            DrawConstructionProgressBar(Actor);
        }
        else if (bShowHoverConstructionProgressBars && Actor == PlayerController->GetHoveredActor())
        {
            // Draw progress bar for hovered actor.
            DrawConstructionProgressBar(Actor);
        }
        else if (bShowSelectionConstructionProgressBars && PlayerController->GetSelectedActors().Contains(Actor))
        {
            // Draw progress bars for selected actors.
            DrawConstructionProgressBar(Actor);
        }
        else
        {
            HideConstructionProgressBar(Actor);
        }
    }
}

void ARTSHUD::DrawConstructionProgressBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    // Check progress.
    URTSConstructionSiteComponent* ConstructionSiteComponent = Actor->FindComponentByClass<URTSConstructionSiteComponent>();

    if (!ConstructionSiteComponent)
    {
        return;
    }

    if (!ConstructionSiteComponent->IsConstructing())
    {
        return;
    }

    const float ProgressPercentage = ConstructionSiteComponent->GetProgressPercentage();

    // Draw progress bar.
    URTSConstructionProgressBarWidgetComponent* ConstructionProgressBarWidgetComponent =
        Actor->FindComponentByClass<URTSConstructionProgressBarWidgetComponent>();

    if (!IsValid(ConstructionProgressBarWidgetComponent))
    {
        return;
    }

    FVector2D Size = GetActorSizeOnScreen(Actor);

    ConstructionProgressBarWidgetComponent->UpdatePositionAndSize(Size);
    ConstructionProgressBarWidgetComponent->SetVisibility(true);
}

void ARTSHUD::HideConstructionProgressBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    URTSConstructionProgressBarWidgetComponent* ConstructionProgressBarWidgetComponent =
        Actor->FindComponentByClass<URTSConstructionProgressBarWidgetComponent>();

    if (!IsValid(ConstructionProgressBarWidgetComponent))
    {
        return;
    }

    ConstructionProgressBarWidgetComponent->SetVisibility(false);
}

void ARTSHUD::DrawHoveredActorWidget()
{
	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);

	if (!PlayerController)
	{
		return;
	}

	AActor* NewHoveredActor = PlayerController->GetHoveredActor();

	if (NewHoveredActor == OldHoveredActor)
	{
		return;
	}

    if (IsValid(OldHoveredActor))
    {
        URTSHoveredActorWidgetComponent* OldHoveredActorWidgetComponent =
            OldHoveredActor->FindComponentByClass<URTSHoveredActorWidgetComponent>();

        if (IsValid(OldHoveredActorWidgetComponent))
        {
            OldHoveredActorWidgetComponent->SetVisibility(false);
        }
    }
    
    if (IsValid(NewHoveredActor))
    {
        URTSHoveredActorWidgetComponent* NewHoveredActorWidgetComponent =
            NewHoveredActor->FindComponentByClass<URTSHoveredActorWidgetComponent>();

        if (IsValid(NewHoveredActorWidgetComponent))
        {
            FVector2D Size = GetActorSizeOnScreen(NewHoveredActor);

            NewHoveredActorWidgetComponent->UpdatePositionAndSize(Size);
            NewHoveredActorWidgetComponent->UpdateData(NewHoveredActor);
            NewHoveredActorWidgetComponent->SetVisibility(true);
        }
    }

    OldHoveredActor = NewHoveredActor;
}

void ARTSHUD::DrawProductionProgressBars()
{
    ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);

    if (!PlayerController)
    {
        return;
    }

    for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
    {
        AActor* Actor = *ActorIt;

        if (!URTSGameplayLibrary::IsFullyVisibleForLocalClient(Actor))
        {
            HideProductionProgressBar(Actor);
            continue;
        }

        // Check override conditions.
        if (bAlwaysShowProductionProgressBars || (bShowHotkeyProductionProgressBars && PlayerController->IsProductionProgressBarHotkeyPressed()))
        {
            // Draw all progress bars.
            DrawProductionProgressBar(Actor);
        }
        else if (bShowHoverProductionProgressBars && Actor == PlayerController->GetHoveredActor())
        {
            // Draw progress bar for hovered actor.
            DrawProductionProgressBar(Actor);
        }
        else if (bShowSelectionProductionProgressBars && PlayerController->GetSelectedActors().Contains(Actor))
        {
            // Draw progress bars for selected actors.
            DrawProductionProgressBar(Actor);
        }
        else
        {
            HideProductionProgressBar(Actor);
        }
    }
}

void ARTSHUD::DrawProductionProgressBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    // Check progress.
    URTSProductionComponent* ProductionComponent = Actor->FindComponentByClass<URTSProductionComponent>();

    if (!ProductionComponent)
    {
        return;
    }

    if (!ProductionComponent->IsProducing())
    {
        return;
    }

    const float ProgressPercentage = ProductionComponent->GetProgressPercentage();

    // Draw progress bar.
    URTSProductionProgressBarWidgetComponent* ProductionProgressBarWidgetComponent =
        Actor->FindComponentByClass<URTSProductionProgressBarWidgetComponent>();

    if (!IsValid(ProductionProgressBarWidgetComponent))
    {
        return;
    }

    FVector2D Size = GetActorSizeOnScreen(Actor);

    ProductionProgressBarWidgetComponent->UpdatePositionAndSize(Size);
    ProductionProgressBarWidgetComponent->SetVisibility(true);
}

void ARTSHUD::HideProductionProgressBar(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    URTSProductionProgressBarWidgetComponent* ProductionProgressBarWidgetComponent =
        Actor->FindComponentByClass<URTSProductionProgressBarWidgetComponent>();

    if (!IsValid(ProductionProgressBarWidgetComponent))
    {
        return;
    }

    ProductionProgressBarWidgetComponent->SetVisibility(false);
}

/*void ARTSHUD::DrawProductionQueue()
{
	ProductionQueueIcons.Reset();

	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);
	if (!PlayerController)
	{
		return;
	}

	// Find the first selected building with a production component.
	AActor* ProductionActor = nullptr;
	URTSProductionComponent* ProductionComponent = nullptr;

	for (AActor* SelectedActor : PlayerController->GetSelectedActors())
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		auto* ProdComp = SelectedActor->FindComponentByClass<URTSProductionComponent>();
		if (ProdComp && ProdComp->IsProducing())
		{
			ProductionActor = SelectedActor;
			ProductionComponent = ProdComp;
			break;
		}
	}

	if (!ProductionComponent)
	{
		return;
	}

	// Apply DPI scale to match UMG widget scaling.
	const float DPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(Canvas->SizeX, Canvas->SizeY));
	const float ScaledIconSize = ProductionQueueIconSize * DPIScale;
	const float ScaledIconPadding = ProductionQueueIconPadding * DPIScale;
	const float ScaledBottomOffset = ProductionQueueBottomOffset * DPIScale;

	const int32 QueueCount = ProductionComponent->GetQueueCount();

	for (int32 QueueIndex = 0; QueueIndex < QueueCount; ++QueueIndex)
	{
		TArray<TSubclassOf<AActor>> QueuedProducts = ProductionComponent->GetQueuedProducts(QueueIndex);

		if (QueuedProducts.Num() == 0)
		{
			continue;
		}

		// Layout: centered horizontally, near bottom of screen.
		const float TotalWidth = QueuedProducts.Num() * ScaledIconSize +
			(QueuedProducts.Num() - 1) * ScaledIconPadding;
		const float StartX = (Canvas->SizeX - TotalWidth) * 0.5f;
		const float StartY = Canvas->SizeY - ScaledBottomOffset - ScaledIconSize;

		for (int32 ProductIndex = 0; ProductIndex < QueuedProducts.Num(); ++ProductIndex)
		{
			TSubclassOf<AActor> ProductClass = QueuedProducts[ProductIndex];
			if (!ProductClass)
			{
				continue;
			}

			const float IconX = StartX + ProductIndex * (ScaledIconSize + ScaledIconPadding);
			const float IconY = StartY;

			// Draw dark background.
			FCanvasTileItem BackgroundTile(
				FVector2D(IconX, IconY),
				FVector2D(ScaledIconSize, ScaledIconSize),
				FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
			BackgroundTile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(BackgroundTile);

			// Get portrait texture from the product class CDO.
			URTSPortraitComponent* PortraitComp =
				URTSGameplayLibrary::FindDefaultComponentByClass<URTSPortraitComponent>(ProductClass);

			if (PortraitComp && PortraitComp->GetPortrait())
			{
				UTexture2D* Portrait = PortraitComp->GetPortrait();
				const FTexture* PortraitResource = Portrait->GetResource();
				if (PortraitResource)
				{
					FCanvasTileItem PortraitTile(
						FVector2D(IconX, IconY),
						PortraitResource,
						FVector2D(ScaledIconSize, ScaledIconSize),
						FVector2D(0.0f, 0.0f),
						FVector2D(1.0f, 1.0f),
						FLinearColor::White);
					PortraitTile.BlendMode = SE_BLEND_Translucent;
					Canvas->DrawItem(PortraitTile);
				}
			}

			// Draw progress overlay for the first item (currently producing).
			if (ProductIndex == 0)
			{
				const float Progress = ProductionComponent->GetProgressPercentage(QueueIndex);
				const float ProgressHeight = ScaledIconSize * Progress;
				const float ProgressY = IconY + ScaledIconSize - ProgressHeight;

				FCanvasTileItem ProgressTile(
					FVector2D(IconX, ProgressY),
					FVector2D(ScaledIconSize, ProgressHeight),
					FLinearColor(0.0f, 0.8f, 0.0f, 0.35f));
				ProgressTile.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(ProgressTile);
			}

			// Draw border.
			const FLinearColor BorderColor = (ProductIndex == 0)
				? FLinearColor(0.0f, 0.8f, 0.0f, 0.8f)
				: FLinearColor(0.5f, 0.5f, 0.5f, 0.6f);

			DrawLine(IconX, IconY, IconX + ScaledIconSize, IconY, BorderColor);
			DrawLine(IconX, IconY + ScaledIconSize, IconX + ScaledIconSize, IconY + ScaledIconSize, BorderColor);
			DrawLine(IconX, IconY, IconX, IconY + ScaledIconSize, BorderColor);
			DrawLine(IconX + ScaledIconSize, IconY, IconX + ScaledIconSize, IconY + ScaledIconSize, BorderColor);

			// Register hit box for click detection.
			FName HitBoxName = *FString::Printf(TEXT("ProdQueue_%d_%d"), QueueIndex, ProductIndex);
			AddHitBox(
				FVector2D(IconX, IconY),
				FVector2D(ScaledIconSize, ScaledIconSize),
				HitBoxName,
				false,
				0);

			// Cache icon data for reference.
			FRTSProductionQueueIconData IconData;
			IconData.Position = FVector2D(IconX, IconY);
			IconData.Size = FVector2D(ScaledIconSize, ScaledIconSize);
			IconData.ProductionActor = ProductionActor;
			IconData.QueueIndex = QueueIndex;
			IconData.ProductIndex = ProductIndex;
			ProductionQueueIcons.Add(IconData);
		}
	}
}*/

bool ARTSHUD::IsPositionOnProductionQueue(float ScreenX, float ScreenY) const
{
	for (const FRTSProductionQueueIconData& IconData : ProductionQueueIcons)
	{
		if (ScreenX >= IconData.Position.X && ScreenX <= IconData.Position.X + IconData.Size.X &&
			ScreenY >= IconData.Position.Y && ScreenY <= IconData.Position.Y + IconData.Size.Y)
		{
			return true;
		}
	}
	return false;
}

void ARTSHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);

	const FString BoxString = BoxName.ToString();

	// Handle production queue clicks.
	/*if (BoxString.StartsWith(TEXT("ProdQueue_")))
	{
		// Parse "ProdQueue_{QueueIndex}_{ProductIndex}".
		int32 QueueIndex = 0;
		int32 ProductIndex = 0;

		TArray<FString> Parts;
		BoxString.ParseIntoArray(Parts, TEXT("_"));
		if (Parts.Num() >= 3)
		{
			QueueIndex = FCString::Atoi(*Parts[1]);
			ProductIndex = FCString::Atoi(*Parts[2]);
		}

		// Find matching cached icon data.
		for (const FRTSProductionQueueIconData& IconData : ProductionQueueIcons)
		{
			if (IconData.QueueIndex == QueueIndex && IconData.ProductIndex == ProductIndex)
			{
				ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);
				if (PlayerController)
				{
					PlayerController->RequestCancelProductionAt(IconData.ProductionActor, QueueIndex, ProductIndex);
				}
				break;
			}
		}
		return;
	}*/

	// Handle ability icon clicks.
	if (BoxString.StartsWith(TEXT("Ability_")))
	{
		int32 AbilityIndex = FCString::Atoi(*BoxString.Mid(8));

		for (const FRTSAbilityIconData& IconData : AbilityIcons)
		{
			if (IconData.AbilityIndex == AbilityIndex)
			{
				ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);
				if (PlayerController)
				{
					PlayerController->BeginAbilityTargeting(IconData.AbilityActor, AbilityIndex);
				}
				break;
			}
		}
		return;
	}
}

void ARTSHUD::DrawAbilityIcons()
{
	AbilityIcons.Reset();

	ARTSPlayerController* PlayerController = Cast<ARTSPlayerController>(PlayerOwner);
	if (!PlayerController)
	{
		return;
	}

	// Find the first selected actor with an ability system component.
	AActor* AbilityActor = nullptr;
	URTSAbilitySystemComponent* AbilitySystem = nullptr;

	for (AActor* SelectedActor : PlayerController->GetSelectedActors())
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		auto* AbilityComp = SelectedActor->FindComponentByClass<URTSAbilitySystemComponent>();
		if (AbilityComp)
		{
			AbilityActor = SelectedActor;
			AbilitySystem = AbilityComp;
			break;
		}
	}

	if (!AbilitySystem)
	{
		return;
	}

	TArray<FRTSAbilityData> Abilities = AbilitySystem->GetAbilities();
	if (Abilities.Num() == 0)
	{
		return;
	}

	URTSPlayerUpgradeComponent* UpgradeComp = PlayerController->FindComponentByClass<URTSPlayerUpgradeComponent>();

	// Apply DPI scale.
	const float DPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(Canvas->SizeX, Canvas->SizeY));
	const float ScaledIconSize = AbilityIconSize * DPIScale;
	const float ScaledIconPadding = AbilityIconPadding * DPIScale;
	const float ScaledBottomOffset = AbilityIconBottomOffset * DPIScale;

	// Layout: centered horizontally, above the production queue.
	const float TotalWidth = Abilities.Num() * ScaledIconSize +
		(Abilities.Num() - 1) * ScaledIconPadding;
	const float StartX = (Canvas->SizeX - TotalWidth) * 0.5f;
	const float StartY = Canvas->SizeY - ScaledBottomOffset - ScaledIconSize;

	for (int32 i = 0; i < Abilities.Num(); ++i)
	{
		const FRTSAbilityData& AbilityData = Abilities[i];
		if (!AbilityData.AbilityClass)
		{
			continue;
		}

		const URTSAbility* AbilityCDO = AbilityData.AbilityClass->GetDefaultObject<URTSAbility>();

		const bool bLocked = AbilityData.RequiredUpgrade &&
			(!IsValid(UpgradeComp) || !UpgradeComp->HasUpgrade(AbilityData.RequiredUpgrade));

		const float IconX = StartX + i * (ScaledIconSize + ScaledIconPadding);
		const float IconY = StartY;

		// Draw dark background.
		FCanvasTileItem BackgroundTile(
			FVector2D(IconX, IconY),
			FVector2D(ScaledIconSize, ScaledIconSize),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		BackgroundTile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BackgroundTile);

		// Draw ability icon texture.
		UTexture2D* Icon = AbilityCDO->GetAbilityIcon();
		if (Icon)
		{
			const FTexture* IconResource = Icon->GetResource();
			if (IconResource)
			{
				const FLinearColor IconTint = bLocked
					? FLinearColor(0.3f, 0.3f, 0.3f, 1.0f)
					: FLinearColor::White;
				FCanvasTileItem IconTile(
					FVector2D(IconX, IconY),
					IconResource,
					FVector2D(ScaledIconSize, ScaledIconSize),
					FVector2D(0.0f, 0.0f),
					FVector2D(1.0f, 1.0f),
					IconTint);
				IconTile.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(IconTile);
			}
		}

		// Draw lock overlay for abilities requiring an unresearched upgrade.
		if (bLocked)
		{
			FCanvasTileItem LockTile(
				FVector2D(IconX, IconY),
				FVector2D(ScaledIconSize, ScaledIconSize),
				FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
			LockTile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(LockTile);
		}

		// Draw cooldown overlay.
		if (AbilityData.RemainingCooldown > 0 && AbilityCDO->GetCooldown() > 0)
		{
			const float CooldownFraction = AbilityData.RemainingCooldown / AbilityCDO->GetCooldown();
			const float OverlayHeight = ScaledIconSize * CooldownFraction;

			FCanvasTileItem CooldownTile(
				FVector2D(IconX, IconY),
				FVector2D(ScaledIconSize, OverlayHeight),
				FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
			CooldownTile.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(CooldownTile);
		}

		// Draw border.
		const bool bReady = AbilityData.RemainingCooldown <= 0;
		const bool bIsTargeting = PlayerController->IsAbilityTargeting() && PlayerController->GetAbilityTargetingIndex() == i;
		FLinearColor BorderColor;

		if (bIsTargeting)
		{
			BorderColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f); // Gold when targeting.
		}
		else if (bReady)
		{
			BorderColor = FLinearColor(0.0f, 0.8f, 0.0f, 0.8f); // Green when ready.
		}
		else
		{
			BorderColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.6f); // Gray when on cooldown.
		}

		DrawLine(IconX, IconY, IconX + ScaledIconSize, IconY, BorderColor);
		DrawLine(IconX, IconY + ScaledIconSize, IconX + ScaledIconSize, IconY + ScaledIconSize, BorderColor);
		DrawLine(IconX, IconY, IconX, IconY + ScaledIconSize, BorderColor);
		DrawLine(IconX + ScaledIconSize, IconY, IconX + ScaledIconSize, IconY + ScaledIconSize, BorderColor);

		// Register hit box and cache icon data — skip for locked abilities.
		if (!bLocked)
		{
			FName HitBoxName = *FString::Printf(TEXT("Ability_%d"), i);
			AddHitBox(
				FVector2D(IconX, IconY),
				FVector2D(ScaledIconSize, ScaledIconSize),
				HitBoxName,
				false,
				0);

			FRTSAbilityIconData IconDataEntry;
			IconDataEntry.Position = FVector2D(IconX, IconY);
			IconDataEntry.Size = FVector2D(ScaledIconSize, ScaledIconSize);
			IconDataEntry.AbilityActor = AbilityActor;
			IconDataEntry.AbilityIndex = i;
			AbilityIcons.Add(IconDataEntry);
		}
	}
}

bool ARTSHUD::IsPositionOnAbilityIcons(float ScreenX, float ScreenY) const
{
	for (const FRTSAbilityIconData& IconData : AbilityIcons)
	{
		if (ScreenX >= IconData.Position.X && ScreenX <= IconData.Position.X + IconData.Size.X &&
			ScreenY >= IconData.Position.Y && ScreenY <= IconData.Position.Y + IconData.Size.Y)
		{
			return true;
		}
	}
	return false;
}
