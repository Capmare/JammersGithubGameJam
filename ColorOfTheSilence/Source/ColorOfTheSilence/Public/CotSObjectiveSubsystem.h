// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CotSObjectiveSubsystem.generated.h"

class AKeySpot;
class AKeyPickup;
class UWorld;

/** How the game ended. */
UENUM(BlueprintType)
enum class EGameOutcome : uint8
{
	None,
	Won,
	Lost
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCotSKeysChanged, int32, Keys, int32, KeysRequired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCotSGameWon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCotSGameLost);

/**
 * Tracks the keys and the end of the game.
 *
 * This subsystem lives on the GameInstance. 
 * The player can pick up keys in one level and place them in the hub in another level, do not think it will be necesary tho.
 *
 * Rules:
 * - AKeySpot actors register themself here when they begin play.
 * - On each world begin play, this class picks random spots and spawns keys on them.
 * - The game spawns KeysRequired keys in one run. It spawns no more.
 * - AKeyPickup calls ReportKeyCollected when the player presses the interact key on it.
 * - AHubKeyLock calls ReportKeyPlacedInHub when the player puts a key in it.
 * - The monster code calls ReportPlayerCaught when it catches the player.
 */
UCLASS()
class COLOROFTHESILENCE_API UCotSObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UCotSObjectiveSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;



	UPROPERTY(BlueprintReadWrite, Category = "CotS|Objective")
	int32 KeysRequired{3};

	UPROPERTY(BlueprintReadWrite, Category = "CotS|Objective")
	TSubclassOf<AKeyPickup> DefaultPickupClass{};

	UPROPERTY(BlueprintReadWrite, Category = "CotS|Objective")
	int32 RandomSeed{-1};


	// Bindings
	UPROPERTY(BlueprintAssignable, Category = "CotS|Objective")
	FOnCotSKeysChanged OnKeyCollected;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Objective")
	FOnCotSKeysChanged OnKeyPlacedInHub;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Objective")
	FOnCotSGameWon OnGameWon;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Objective")
	FOnCotSGameLost OnGameLost;


	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	void RegisterKeySpot(AKeySpot* Spot);
	
	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	void UnregisterKeySpot(const AKeySpot* Spot);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	bool IsSpotActive(const AKeySpot* Spot) const;

	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	void SpawnKeysForRegisteredSpots();



	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	bool ReportKeyCollected(const AKeyPickup* Key);

	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	bool ReportKeyPlacedInHub();

	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	void ReportPlayerCaught();

	UFUNCTION(BlueprintCallable, Category = "CotS|Objective")
	void ResetObjectives();


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	int32 GetCollectedKeyCount() const { return CollectedKeys; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	int32 GetPlacedKeyCount() const { return PlacedKeys; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	int32 GetHeldKeyCount() const { return FMath::Max(0, CollectedKeys - PlacedKeys); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	int32 GetUnspawnedKeyCount() const { return FMath::Max(0, KeysRequired - KeysSpawned); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	bool HasPlacedAllKeys() const { return PlacedKeys >= KeysRequired; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	bool IsGameFinished() const { return bGameFinished; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Objective")
	EGameOutcome GetOutcome() const { return Outcome; }

private:
	void ScheduleRoll(UWorld* World);
	void PruneDeadSpots();
	bool ActivateSpot(AKeySpot* Spot);
	bool ContainsSpot(const TArray<TWeakObjectPtr<AKeySpot>>& List, const AKeySpot* Spot) const;

	bool bGameFinished{};
	EGameOutcome Outcome{EGameOutcome::None};

	int32 KeysSpawned{};
	int32 CollectedKeys{};
	int32 PlacedKeys{};

	FRandomStream SpotRandomizer{};
	int32 UsedSeed{INDEX_NONE};
	FTimerHandle RollTimerHandle{};
	TWeakObjectPtr<UWorld> RollWorld{};
	TWeakObjectPtr<UWorld> RolledWorld{};

	TArray<TWeakObjectPtr<AKeySpot>> RegisteredSpots;
	TArray<TWeakObjectPtr<AKeySpot>> ActiveSpots;
};
