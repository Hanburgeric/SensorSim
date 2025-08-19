#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BaseCameraSensor.generated.h"

// Forward declarations
class UCineCameraComponent;
class UCineCaptureComponent2D;

UCLASS(Abstract)
class UESENSORS_API UBaseCameraSensor : public USceneComponent
{
	GENERATED_BODY()

public:
	UBaseCameraSensor();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Sensor")
	TObjectPtr<UCineCameraComponent> CameraComponent{ nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Sensor")
	TObjectPtr<UCineCaptureComponent2D> CaptureComponent{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Render Target", meta = (ClampMin = "1"))
	int32 RenderTargetWidth{ 1920 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Render Target", meta = (ClampMin = "1"))
	int32 RenderTargetHeight{ 1080 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Render Target")
	TEnumAsByte<EPixelFormat> RenderTargetPixelFormat{ PF_B8G8R8A8 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Render Target")
	bool bForceRenderTargetLinearGamma{ false };
};
