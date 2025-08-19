#include "Sensors/Camera/SemanticSegmentationCameraSensor.h"

// UESensors
#include "CineCameraComponent.h"
#include "Misc/SemanticLabelData.h"
#include "Utils/MaterialUtils.h"

DEFINE_LOG_CATEGORY(LogSemanticSegmentationCameraSensor);

USemanticSegmentationCameraSensor::USemanticSegmentationCameraSensor()
{
	// TODO: disable irrelevant show flags for performance optimization
}

void USemanticSegmentationCameraSensor::BeginPlay()
{
	Super::BeginPlay();

	// Check whether custom depth with stencil has been enabled in project settings
	if (!IsCustomDepthWithStencilEnabled())
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Custom depth with stencil is not enabled in project settings; ")
			TEXT("camera will output a regular image.")
		);
	}
	
	// Create a 1D LUT using the provided semantic label data
	if (AreSemanticLabelsValid())
	{
		SemanticLabelLUT = CreateLUTFromSemanticLabels();
		check(SemanticLabelLUT);
	}
	else
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Semantic label data is null or contains zero entries; ")
			TEXT("segmentation material will use default texture and scalar parameters.")
		);
	}

	// Create a dynamic instance of the post process segmentation material
	if (IsBaseSegmentationMaterialValid())
	{
		SegmentationMaterial = CreateSegmentationMaterialFromBase();
		check(SegmentationMaterial);

		// Set the material's parameters using the LUT's properties
		if (IsLUTValid())
		{
			SetSegmentationMaterialParameters();
		}
		else
		{
			UE_LOG(
				LogSemanticSegmentationCameraSensor, Warning,
				TEXT("Semantic label lookup texture is null or zero-width; ")
				TEXT("segmentation material will use the default texture and scalar parmeters.")
			);
		}

		// Add the segmentation material to the camera
		CameraComponent->AddOrUpdateBlendable(SegmentationMaterial);
	}
	else
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Base segmentation material is null or not a post process material; ")
			TEXT("camera will output a regular image.")
		);
	}
}

bool USemanticSegmentationCameraSensor::IsCustomDepthWithStencilEnabled()
{
	return GetCustomDepthMode() == ECustomDepthMode::EnabledWithStencil;
}

bool USemanticSegmentationCameraSensor::AreSemanticLabelsValid() const
{
	return SemanticLabels && !SemanticLabels->SemanticLabels.IsEmpty();
}

UTexture2D* USemanticSegmentationCameraSensor::CreateLUTFromSemanticLabels()
{
	// Create a new 2D texture
	UTexture2D* LUT{ NewObject<UTexture2D>(this) };
	check(LUT);

	// Set LOD group as a color lookup table; prevents compression and mip generation
	LUT->LODGroup = TEXTUREGROUP_ColorLookupTable;

	// Disable gamma correction, as the LUT is data, not color for display; they must be in linear space
	LUT->SRGB = false;

	// Do not blend adjacent pixels, as this would mix colors from different semantic labels
	LUT->Filter = TF_Nearest;

	// Initialize texture
	const int32 Width{ SemanticLabels->SemanticLabels.Num() };
	const int32 Height{ 1 };
	LUT->Source.Init(Width, Height, 1, 1, TSF_BGRA8);

	// Edit the texture
	uint8* MipData{ LUT->Source.LockMip(0) };
	for (int32 x{ 0 }; x < Width; ++x)
	{
		// Get the color corresponding to the semantic label at the current texture index
		const FColor& PixelColor{
			SemanticLabels->SemanticLabels[x].ConvertedColor
		};

		// Write the color to the texture data (4 bytes per pixel)
		const int32 MipDataIndex{ x * 4 };
		MipData[MipDataIndex + 0] = PixelColor.B;
		MipData[MipDataIndex + 1] = PixelColor.G;
		MipData[MipDataIndex + 2] = PixelColor.R;
		MipData[MipDataIndex + 3] = PixelColor.A;
	}
	LUT->Source.UnlockMip(0);

	// Update the texture
	LUT->UpdateResource();

	return LUT;
}

bool USemanticSegmentationCameraSensor::IsBaseSegmentationMaterialValid() const
{
	return BaseSegmentationMaterial && BaseSegmentationMaterial->IsPostProcessMaterial();
}

UMaterialInstanceDynamic* USemanticSegmentationCameraSensor::CreateSegmentationMaterialFromBase()
{
	// Create a dynamic material instance using the base segmentation material
	UMaterialInstanceDynamic* BaseMaterialInstance{
		UMaterialInstanceDynamic::Create(
			BaseSegmentationMaterial, this
		)
	};
	check(BaseMaterialInstance);

	return BaseMaterialInstance;
}

bool USemanticSegmentationCameraSensor::IsLUTValid() const
{
	return SemanticLabelLUT && SemanticLabelLUT->GetSizeX() > 0;
}

void USemanticSegmentationCameraSensor::SetSegmentationMaterialParameters() const
{
	// Find a texture parameter with a name matching the one set in LUTParameterName
	bool bFoundLUTParameter{
		UMaterialUtils::DoesMaterialHaveParameter(
			BaseSegmentationMaterial,
			EMaterialParameterType::Texture,
			LUTParameterName
		)
	};

	// Set the texture parameter with the matching name to the LUT
	if (bFoundLUTParameter)
	{
		SegmentationMaterial->SetTextureParameterValue(
			LUTParameterName,
			SemanticLabelLUT
		);
	}
	else
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Segmentation material has no texture parameter named \"%s\"; ")
			TEXT("the default texture will be used."),
			*LUTParameterName.ToString()
		);
	}
	
	// Find a scalar parameter with a name matching the one set in LUTInvWidthParameterName
	bool bFoundLUTInvWidthParameter{
		UMaterialUtils::DoesMaterialHaveParameter(
			BaseSegmentationMaterial,
			EMaterialParameterType::Scalar,
			LUTInvWidthParameterName
		)
	};

	// Set the scalar parameter with the matching name to the inverse of the LUT's width
	if (bFoundLUTInvWidthParameter)
	{
		SegmentationMaterial->SetScalarParameterValue(
			LUTInvWidthParameterName,
			1.0F / static_cast<float>(SemanticLabels->SemanticLabels.Num())
		);
	}
	else
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Segmentation material has no scalar parameter named \"%s\"; ")
			TEXT("the default value will be used."),
			*LUTInvWidthParameterName.ToString()
		);
	}
}
