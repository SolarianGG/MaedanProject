#include "Vision/RTSFogOfWarActor.h"

#include "EngineUtils.h"
#include "RHI.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "RTSLog.h"
#include "Vision/RTSVisionInfo.h"
#include "Vision/RTSVisionVolume.h"


ARTSFogOfWarActor::ARTSFogOfWarActor(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARTSFogOfWarActor::Initialize(ARTSVisionVolume* InVisionVolume)
{
	// Get vision size.
	VisionVolume = InVisionVolume;

	if (!VisionVolume)
	{
		UE_LOG(LogRTS, Warning, TEXT("No vision volume found, won't update fog of war."));
		return;
	}

	if (!FogOfWarVolume)
	{
		UE_LOG(LogRTS, Warning, TEXT("No fog of war volume found, won't render fog of war."));
		return;
	}

	// Setup fog of war buffer.
	int32 SizeInTiles = VisionVolume->GetSizeInTiles();
	FVector SizeInWorld = VisionVolume->GetSizeInWorld();

	int32 BufferSize = SizeInTiles * SizeInTiles * 4;
	FogOfWarTextureBuffer = new uint8[BufferSize];
	FogOfWarBlurBuffer = new uint8[BufferSize];

	// Initialize buffer to Unknown state (30 grey, alpha 0).
	for (int32 i = 0; i < SizeInTiles * SizeInTiles; ++i)
	{
		FogOfWarTextureBuffer[i * 4 + 0] = 30; // Blue
		FogOfWarTextureBuffer[i * 4 + 1] = 30; // Green
		FogOfWarTextureBuffer[i * 4 + 2] = 30; // Red
		FogOfWarTextureBuffer[i * 4 + 3] = 0;  // Alpha
	}

	// Setup fog of war texture.
	FogOfWarTexture = UTexture2D::CreateTransient(SizeInTiles, SizeInTiles);
	FogOfWarTexture->Filter = TF_Bilinear;
	FogOfWarTexture->SRGB = 0;

#if WITH_EDITORONLY_DATA
	FogOfWarTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
#endif

	FogOfWarTexture->AddToRoot();
	FogOfWarTexture->UpdateResource();

	FogOfWarUpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, SizeInTiles, SizeInTiles);

	// Setup fog of war material.
	FogOfWarMaterialInstance = UMaterialInstanceDynamic::Create(FogOfWarMaterial, nullptr);
	FogOfWarMaterialInstance->SetTextureParameterValue(FName("VisibilityMask"), FogOfWarTexture);
	FogOfWarMaterialInstance->SetScalarParameterValue(FName("OneOverWorldSize"), 1.0f / SizeInWorld.X);
	FogOfWarMaterialInstance->SetScalarParameterValue(FName("OneOverTileSize"), 1.0f / SizeInTiles);

	// Setup fog of war post-process volume.
	FogOfWarVolume->AddOrUpdateBlendable(FogOfWarMaterialInstance);

	// Bug C fix: pass the volume's world centre so the material can compute correct UVs when
	// ARTSVisionVolume is not at world origin.  The material must implement:
	//   UV = (WorldPos.xy - WorldCenter.xy) * OneOverWorldSize + 0.5
	FVector VolumeLocation = VisionVolume->GetActorLocation();
	FogOfWarMaterialInstance->SetVectorParameterValue(FName("WorldCenter"),
		FLinearColor(VolumeLocation.X, VolumeLocation.Y, 0.f, 0.f));

	// Bug A fix: upload the initialised grey buffer immediately so the GPU texture starts at
	// VISION_Unknown grey instead of uninitialised (black) memory, which caused an all-black
	// screen on clients while VisionInfo was still being replicated.
	FogOfWarTexture->UpdateTextureRegions(0, 1, FogOfWarUpdateTextureRegion, SizeInTiles * 4, (uint32)4, FogOfWarTextureBuffer);

	UE_LOG(LogRTS, Log, TEXT("Set up %s with %s."), *GetName(), *VisionVolume->GetName());
}

void ARTSFogOfWarActor::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);

	// Update texture.
	if (!VisionVolume || !FogOfWarVolume || !VisionInfo)
	{
		return;
	}

	int32 SizeInTiles = VisionVolume->GetSizeInTiles();

	for (int32 Y = 0; Y < SizeInTiles; ++Y)
	{
		for (int32 X = 0; X < SizeInTiles; ++X)
		{
			const int32 i = Y * SizeInTiles + X;

			const int32 iBlue = i * 4 + 0;
			const int32 iGreen = i * 4 + 1;
			const int32 iRed = i * 4 + 2;
			const int32 iAlpha = i * 4 + 3;

			// Determine target brightness for this tile.
			uint8 TargetValue;

			switch (VisionInfo->GetVision(X, Y))
			{
			case ERTSVisionState::VISION_Visible:
				TargetValue = 180;
				break;

			case ERTSVisionState::VISION_Known:
				TargetValue = 90;
				break;

			case ERTSVisionState::VISION_Unknown:
			default:
				TargetValue = 30;
				break;
			}

			// Temporal smoothing: lerp current value toward target.
			const uint8 CurrentValue = FogOfWarTextureBuffer[iBlue];
			const uint8 NewValue = (uint8)FMath::Clamp(
				FMath::CeilToInt(FMath::FInterpTo((float)CurrentValue, (float)TargetValue, DeltaTime, FogFadeSpeed)),
				0, 255);

			FogOfWarTextureBuffer[iBlue] = NewValue;
			FogOfWarTextureBuffer[iGreen] = NewValue;
			FogOfWarTextureBuffer[iRed] = NewValue;
			FogOfWarTextureBuffer[iAlpha] = 0;
		}
	}

	// Apply blur for softer edges.
	if (BlurRadius > 0)
	{
		ApplyBlur(SizeInTiles);
	}

	FogOfWarTexture->UpdateTextureRegions(0, 1, FogOfWarUpdateTextureRegion, SizeInTiles * 4, (uint32)4, FogOfWarTextureBuffer);
}

void ARTSFogOfWarActor::ApplyBlur(int32 TextureSize)
{
	const int32 Radius = FMath::Clamp(BlurRadius, 1, 3);
	const int32 KernelSize = 2 * Radius + 1;

	// Horizontal pass: read from TextureBuffer, write to BlurBuffer.
	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			int32 Sum = 0;

			for (int32 K = -Radius; K <= Radius; ++K)
			{
				const int32 SampleX = FMath::Clamp(X + K, 0, TextureSize - 1);
				Sum += FogOfWarTextureBuffer[(Y * TextureSize + SampleX) * 4]; // Blue channel
			}

			const int32 Idx = (Y * TextureSize + X) * 4;
			const uint8 Avg = (uint8)(Sum / KernelSize);
			FogOfWarBlurBuffer[Idx + 0] = Avg;
			FogOfWarBlurBuffer[Idx + 1] = Avg;
			FogOfWarBlurBuffer[Idx + 2] = Avg;
			FogOfWarBlurBuffer[Idx + 3] = 0;
		}
	}

	// Vertical pass: read from BlurBuffer, write back to TextureBuffer.
	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			int32 Sum = 0;

			for (int32 K = -Radius; K <= Radius; ++K)
			{
				const int32 SampleY = FMath::Clamp(Y + K, 0, TextureSize - 1);
				Sum += FogOfWarBlurBuffer[(SampleY * TextureSize + X) * 4]; // Blue channel
			}

			const int32 Idx = (Y * TextureSize + X) * 4;
			const uint8 Avg = (uint8)(Sum / KernelSize);
			FogOfWarTextureBuffer[Idx + 0] = Avg;
			FogOfWarTextureBuffer[Idx + 1] = Avg;
			FogOfWarTextureBuffer[Idx + 2] = Avg;
			FogOfWarTextureBuffer[Idx + 3] = 0;
		}
	}
}

UTexture2D* ARTSFogOfWarActor::GetFogOfWarTexture() const
{
	return FogOfWarTexture;
}

void ARTSFogOfWarActor::SetupVisionInfo(ARTSVisionInfo* InVisionInfo)
{
	VisionInfo = InVisionInfo;
}
