#pragma once

#include "CoreMinimal.h"
#include "BaseCameraSensor.h"
#include "SemanticSegmentationCameraSensor.generated.h"

// Forward declarations
class USemanticLabelData;

DECLARE_LOG_CATEGORY_EXTERN(LogSemanticSegmentationCameraSensor, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UESENSORS_API USemanticSegmentationCameraSensor : public UBaseCameraSensor
{
	GENERATED_BODY()

public:
	USemanticSegmentationCameraSensor();

protected:
	virtual void BeginPlay() override;

protected:
	static bool IsCustomDepthWithStencilEnabled();
	
	bool AreSemanticLabelsValid() const;
	UTexture2D* CreateLUTFromSemanticLabels();

	bool IsBaseSegmentationMaterialValid() const;
	UMaterialInstanceDynamic* CreateSegmentationMaterialFromBase();

	bool IsLUTValid() const;
	void SetSegmentationMaterialParameters() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic Segmentation")
	TObjectPtr<USemanticLabelData> SemanticLabels{ nullptr };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Semantic Segmentation")
	TObjectPtr<UTexture2D> SemanticLabelLUT{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic Segmentation")
	TObjectPtr<UMaterial> BaseSegmentationMaterial{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic Segmentation")
	FName LUTParameterName{ TEXT("LUT") };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Semantic Segmentation")
	FName LUTInvWidthParameterName{ TEXT("LUTInvWidth") };
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Semantic Segmentation")
	TObjectPtr<UMaterialInstanceDynamic> SegmentationMaterial{ nullptr };
};
