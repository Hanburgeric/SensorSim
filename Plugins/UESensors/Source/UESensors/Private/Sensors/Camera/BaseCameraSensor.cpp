#include "Sensors/Camera/BaseCameraSensor.h"

// UE
#include "CineCameraComponent.h"
#include "CineCameraSceneCaptureComponent.h"
#include "Engine/TextureRenderTarget2D.h"

UBaseCameraSensor::UBaseCameraSensor()
	: CameraComponent{ CreateDefaultSubobject<UCineCameraComponent>(TEXT("CameraComponent")) }
	, CaptureComponent{ CreateDefaultSubobject<UCineCaptureComponent2D>(TEXT("CaptureComponent")) }
{
	// Parent the camera component to the scene component such that
	// the former automatically inherits the latter's transform
	CameraComponent->SetupAttachment(this);

	// A cine capture component is required to be parented to a cine camera component
	CaptureComponent->SetupAttachment(CameraComponent);
}

void UBaseCameraSensor::BeginPlay()
{
	Super::BeginPlay();

	// Create a texture render target to which to draw the contents of the capture
	CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>(this);
	check(CaptureComponent->TextureTarget);

	// Clamp the texture dimensions to valid values
	RenderTargetWidth = FMath::Max(1, RenderTargetWidth);
	RenderTargetHeight = FMath::Max(1, RenderTargetHeight);
	
	// Initialize the texture target using the provided properties
	CaptureComponent->TextureTarget->InitCustomFormat(
		RenderTargetWidth, RenderTargetHeight,
		RenderTargetPixelFormat, bForceRenderTargetLinearGamma
	);
}
