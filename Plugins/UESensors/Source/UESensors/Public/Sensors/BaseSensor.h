#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BaseSensor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBaseSensor, Log, All);

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSetSensorEnabled, bool, bNewSensorEnabled);

UCLASS(Abstract, Blueprintable, BlueprintType)
class UESENSORS_API UBaseSensor : public USceneComponent
{
	GENERATED_BODY()

public:	
	UBaseSensor();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	UFUNCTION(BlueprintCallable)
	bool GetSensorEnabled() const;

	UFUNCTION(BlueprintCallable)
	void SetSensorEnabled(bool bNewSensorEnabled);

	UFUNCTION(BlueprintCallable)
	double GetScanFrequency() const;

	UFUNCTION(BlueprintCallable)
	void SetScanFrequency(double NewScanFrequency);

	UFUNCTION(BlueprintCallable)
	double GetScanPeriod() const;

	UFUNCTION(BlueprintCallable)
	void SetScanPeriod(double NewScanPeriod);

	UFUNCTION(BlueprintCallable)
	double GetNextScanTime() const;

protected:
	UFUNCTION(BlueprintNativeEvent)
	void OnEnableSensor();
	virtual void OnEnableSensor_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void OnDisableSensor();
	virtual void OnDisableSensor_Implementation();

	UFUNCTION(BlueprintNativeEvent)
	void PerformScan();
	virtual void PerformScan_Implementation();

public:
	UPROPERTY(BlueprintAssignable)
	FOnSetSensorEnabled OnSetSensorEnabled{};

private:
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetSensorEnabled", BlueprintSetter = "SetSensorEnabled")
	bool bSensorEnabled{ true };

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetScanFrequency", BlueprintSetter = "SetScanFrequency",
		meta = (ClampMin = "0.01", ClampMax = "100.0"))
	double ScanFrequency{ 1.0 };

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetScanPeriod", BlueprintSetter = "SetScanPeriod",
		meta = (ClampMin = "0.01", ClampMax = "100.0"))
	double ScanPeriod{ 1.0 };

	UPROPERTY(VisibleAnywhere, BlueprintGetter = "GetNextScanTime")
	double NextScanTime{ 0.0 };
};
