

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "ThemeData.generated.h"

USTRUCT(BlueprintType)
struct FThemeData : public FTableRowBase
{
	GENERATED_BODY()

public: 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ThemeName = FName("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UStaticMesh*> PossibleAssets = {NULL};

	bool operator==(const FThemeData& OtherItem) const
	{
		if (ThemeName == OtherItem.ThemeName) 
		{
			return true;
		}
		return false;
	}
};
