

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInstanceInterface.h"
#include "MainGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class COLOROFTHESILENCE_API UMainGameInstance : public UGameInstance, public IGameInstanceInterface
{
	GENERATED_BODY()

public:
	UMainGameInstance();

	UDataTable* GetThemeDataTable() override;
	void RandomizeTheme(int32 Room) override;

protected:

	UPROPERTY(EditDefaultsOnly)
	class UDataTable* ThemeDataTable;

};
