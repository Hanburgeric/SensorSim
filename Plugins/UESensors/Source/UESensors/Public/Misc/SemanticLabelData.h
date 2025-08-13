#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SemanticLabelData.generated.h"

USTRUCT(BlueprintType)
struct FSemanticLabel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Tag{ NAME_None };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FColor ConvertedColor{ FColor::Black };
};

UCLASS()
class UESENSORS_API USemanticLabelData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSemanticLabel> SemanticLabels{};
};
