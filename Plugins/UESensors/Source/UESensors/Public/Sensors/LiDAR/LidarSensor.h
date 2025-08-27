#pragma once

#include "CoreMinimal.h"
#include "Sensors/BaseSensor.h"
#include "LidarStrategy.h"
#include "LidarSensor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLiDARSensor, Log, All);

UCLASS(ClassGroup = (Custom))
class UESENSORS_API ULidarSensor : public UBaseSensor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	UFUNCTION(BlueprintCallable)
	int32 GetHorizontalSamples() const;

	UFUNCTION(BlueprintCallable)
	int32 GetVerticalSamples() const;

	UFUNCTION(BlueprintCallable)
	float GetHorizontalFieldOfView() const;

	UFUNCTION(BlueprintCallable)
	void SetHorizontalFieldOfView(float NewHorizontalFieldOfView);

	UFUNCTION(BlueprintCallable)
	float GetHorizontalResolution() const;

	UFUNCTION(BlueprintCallable)
	void SetHorizontalResolution(float NewHorizontalResolution);

	UFUNCTION(BlueprintCallable)
	float GetVerticalFieldOfView() const;

	UFUNCTION(BlueprintCallable)
	void SetVerticalFieldOfView(float NewVerticalFieldOfView);

	UFUNCTION(BlueprintCallable)
	float GetVerticalResolution() const;

	UFUNCTION(BlueprintCallable)
	void SetVerticalResolution(float NewVerticalResolution);

	UFUNCTION(BlueprintCallable)
	const TArray<FVector3f>& GetSampleDirections() const;

	UFUNCTION(BlueprintCallable)
	ELidarScanMode GetScanMode() const;

	UFUNCTION(BlueprintCallable)
	void SetScanMode(ELidarScanMode NewScanMode);

	UFUNCTION(BlueprintCallable)
	const TArray<FLidarPoint>& GetScanData() const;

protected:
	virtual void ExecuteScan_Implementation() override;

private:
	void RebuildSampleDirections();
	void ReinitializeScanStrategy();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinRange{ 150.0F };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxRange{ 500000.0F };

private:
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetHorizontalFieldOfView", BlueprintSetter = "SetHorizontalFieldOfView", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float HorizontalFieldOfView{ 90.0F };

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetHorizontalResolution", BlueprintSetter = "SetHorizontalResolution", meta = (ClampMin = "0.01"))
	float HorizontalResolution{ 2.0F };

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetVerticalFieldOfView", BlueprintSetter = "SetVerticalFieldOfView", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float VerticalFieldOfView{ 45.0F };

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetVerticalResolution", BlueprintSetter = "SetVerticalResolution", meta = (ClampMin = "0.01"))
	float VerticalResolution{ 1.0F };

	UPROPERTY(BlueprintGetter = "GetSampleDirections")
	TArray<FVector3f> SampleDirections{};

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetScanMode", BlueprintSetter = "SetScanMode")
	ELidarScanMode ScanMode{ ELidarScanMode::ParallelFor };

	TUniquePtr<uesensors::lidar::Strategy> ScanStrategy{ nullptr };

	UPROPERTY(BlueprintGetter = "GetScanData")
	TArray<FLidarPoint> ScanData{};
};
