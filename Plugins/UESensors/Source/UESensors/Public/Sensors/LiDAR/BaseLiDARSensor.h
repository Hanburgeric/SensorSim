#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BaseLiDARSensor.generated.h"

USTRUCT(BlueprintType)
struct FLiDARAxisProperties
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FOV",
			  meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float FieldOfView{ 360.0F };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FOV",
			  meta = (ClampMin = "-360.0", ClampMax = "360.0"))
	float FieldOfViewOffset{ 0.0F };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolution")
	bool bUseAngularResolution{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolution",
			  meta = (EditCondition = "!bUseAngularResolution", ClampMin = "0"))
	int32 NumberOfSamples{ 360 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resolution",
			  meta = (EditCondition = "bUseAngularResolution", ClampMin = "0.001"))
	float AngularResolution{ 1.0F };
};

UCLASS(Abstract)
class UESENSORS_API UBaseLiDARSensor : public USceneComponent
{
	GENERATED_BODY()

public:
	UBaseLiDARSensor();

protected:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Axis Properties")
	FLiDARAxisProperties HorizontalAxisProperties{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Axis Properties")
	FLiDARAxisProperties VerticalAxisProperties{};

	UPROPERTY(BlueprintReadOnly, Category = "Laser Directions")
	TArray<FVector> AxisDefinedLaserDirections{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Laser Directions")
	TArray<FVector> AdditionalLaserDirections{};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Laser Directions")
	TArray<FVector> LaserDirections{};
};
