

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CotSDifficulty.h"
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

	UFUNCTION(BlueprintCallable, Category = "CotS|Difficulty")
	void SetDifficulty(ECotSDifficulty NewDifficulty) { Difficulty = NewDifficulty; bDifficultySet = true; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Difficulty")
	ECotSDifficulty GetDifficulty() const { return Difficulty; }

	// False until a menu calls SetDifficulty
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Difficulty")
	bool IsDifficultySet() const { return bDifficultySet; }

protected:

	UPROPERTY(EditDefaultsOnly)
	class UDataTable* ThemeDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CotS|Difficulty")
	ECotSDifficulty Difficulty{ECotSDifficulty::Normal};

	bool bDifficultySet{};

	UFUNCTION()
	void OnGameLoaded(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);
};
