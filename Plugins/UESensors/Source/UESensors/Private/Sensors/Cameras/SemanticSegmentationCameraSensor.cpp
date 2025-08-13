#include "Sensors/Cameras/SemanticSegmentationCameraSensor.h"

// UESensors
#include "CineCameraComponent.h"
#include "Misc/SemanticLabelData.h"
#include "Utils/MaterialUtils.h"

DEFINE_LOG_CATEGORY(LogSemanticSegmentationCameraSensor);

USemanticSegmentationCameraSensor::USemanticSegmentationCameraSensor()
{
}

void USemanticSegmentationCameraSensor::BeginPlay()
{
	Super::BeginPlay();

	if (CanSegmentImage())
	{
		InitializeSemanticSegmentationComponents();
	}
	else
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Condition(s) for performing semantic segmentation")
			TEXT("has/have not been met; the component will output a regular image.")
		);
	}
}

bool USemanticSegmentationCameraSensor::CanSegmentImage() const
{
	return IsCustomDepthWithStencilEnabled() && AreSemanticLabelsValid() && IsBaseSegmentationMaterialValid();
}

bool USemanticSegmentationCameraSensor::IsCustomDepthWithStencilEnabled()
{
	if (GetCustomDepthMode() != ECustomDepthMode::EnabledWithStencil)
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Custom depth with stencil is not enabled in project settings.")
		);

		return false;
	}

	return true;
}

bool USemanticSegmentationCameraSensor::AreSemanticLabelsValid() const
{
	if (!SemanticLabels)
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("No semantic label data asset was provided.")
		);

		return false;
	}

	// Disallow using a data asset with no entries, as this will result in
	// attempting to create a LUT with invalid dimensions (i.e. 1x0)
	if (SemanticLabels->SemanticLabels.IsEmpty())
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Semantic label data asset \"%s\" was provided, but contains no data."),
			*SemanticLabels->GetName()
		);

		return false;
	}

	return true;
}

bool USemanticSegmentationCameraSensor::IsBaseSegmentationMaterialValid() const
{
	if (!BaseSegmentationMaterial)
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("No base segmentation material was provided.")
		);

		return false;
	}

	// Make sure that the material is a post process material
	if (!BaseSegmentationMaterial->IsPostProcessMaterial())
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Base segmentation material \"%s\" was provided, ")
			TEXT("but is not a post process material."),
			*BaseSegmentationMaterial->GetName()
		);

		return false;
	}

	// Find a texture parameter with a name matching the one set in LUTParameterName
	bool bFoundLUTParameter{
		UMaterialUtils::DoesMaterialHaveParameter(
			BaseSegmentationMaterial,
			EMaterialParameterType::Texture,
			LUTParameterName
		)
	};
	if (!bFoundLUTParameter)
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Base segmentation material \"%s\" was provided, ")
			TEXT("but has no texture parameter named \"%s\"."),
			*BaseSegmentationMaterial->GetName(),
			*LUTParameterName.ToString()
		);

		return false;
	}

	// Find a scalar parameter with a name matching the one set in LUTInvWidthParameterName
	bool bFoundLUTInvWidthParameter{
		UMaterialUtils::DoesMaterialHaveParameter(
			BaseSegmentationMaterial,
			EMaterialParameterType::Scalar,
			LUTInvWidthParameterName
		)
	};
	if (!bFoundLUTInvWidthParameter)
	{
		UE_LOG(
			LogSemanticSegmentationCameraSensor, Warning,
			TEXT("Base segmentation material \"%s\" was provided, ")
			TEXT("but has no scalar parameter named \"%s\"."),
			*BaseSegmentationMaterial->GetName(),
			*LUTInvWidthParameterName.ToString()
		);

		return false;
	}

	return true;
}

void USemanticSegmentationCameraSensor::InitializeSemanticSegmentationComponents()
{
	// Create a 1D LUT from the semantic labels
	SemanticLabelLUT = CreateLUTFromSemanticLabels();

	// Create a dynamic instance of the segmentation material
	SegmentationMaterialInstance = CreateSegmentationMaterialInstance();

	// Add the segmentation material to the camera
	CameraComponent->AddOrUpdateBlendable(SegmentationMaterialInstance);
}

UTexture2D* USemanticSegmentationCameraSensor::CreateLUTFromSemanticLabels()
{
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

UMaterialInstanceDynamic* USemanticSegmentationCameraSensor::CreateSegmentationMaterialInstance()
{
	UMaterialInstanceDynamic* DynamicMaterialInstance{
		UMaterialInstanceDynamic::Create(
			BaseSegmentationMaterial, this
		)
	};
	check(DynamicMaterialInstance);

	// Set the texture parameter with the matching name to the LUT
	SegmentationMaterialInstance->SetTextureParameterValue(
		LUTParameterName, SemanticLabelLUT
	);

	// Set the scalar parameter with the matching name to the inverse of the LUT's width
	SegmentationMaterialInstance->SetScalarParameterValue(
		LUTInvWidthParameterName,
		1.0F / static_cast<float>(SemanticLabels->SemanticLabels.Num())
	);

	return DynamicMaterialInstance;
}
