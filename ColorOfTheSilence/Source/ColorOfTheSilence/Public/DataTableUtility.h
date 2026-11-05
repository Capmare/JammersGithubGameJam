

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ThemeData.h"
#include "DataTableUtility.generated.h"

/**
 * 
 */
UCLASS()
class COLOROFTHESILENCE_API UDataTableUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FThemeData GetThemeData(UObject* WorldContextObject, FName ThemeName);

};
