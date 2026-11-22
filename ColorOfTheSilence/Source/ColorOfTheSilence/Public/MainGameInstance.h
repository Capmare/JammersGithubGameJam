

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInstanceInterface.h"
#include "GameFramework/SaveGame.h"
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

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveGame(FString SaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadGame(FString SaveSlotName);

protected:

	UPROPERTY(EditDefaultsOnly)
	class UDataTable* ThemeDataTable;

	UFUNCTION()
	void OnGameLoaded(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);
};
