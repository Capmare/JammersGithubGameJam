#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CotSMonsterDirector.generated.h"

class APawn;
class AMonsterCharacter;
class ASectionVolume;
class AMonsterSpawnPoint;

UCLASS()
class COLOROFTHESILENCE_API UCotSMonsterDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCotSMonsterDirector* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "CotS|Director", meta = (WorldContext = "WorldContextObject"))
	static void SendHint(const UObject* WorldContextObject, FVector Location, float Strength = 1.f, FName Source = NAME_None);

	UCotSMonsterDirector();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCotSMonsterDirector, STATGROUP_Tickables); }

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	void SetPlayerSection(int32 Section);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	int32 GetPlayerSection() const { return PlayerSection; }

	// Sound only reaches the monster while the player is in the sound world
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	bool IsSoundWorldActive() const;

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	void GiveHint(FVector Location, float Strength = 1.f, FName Source = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	void ReportNoise(FVector Location, float Loudness = 1.f);

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	void GetHint(int32& OutSection, bool& bOutRetreat, bool& bOutHasLocation, FVector& OutLocation, float& OutStrength);

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	int32 DropHintsNear(FVector Location);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	int32 GetSectionAtLocation(FVector Location) const;

	void RegisterSectionVolume(ASectionVolume* Volume);
	void UnregisterSectionVolume(const ASectionVolume* Volume);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	float GetPressure() const { return Pressure; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	bool IsRetreating() const { return bRetreating; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	bool IsPlayerDetected() const { return bPlayerDetected; }

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	void NotifyPlayerFound();

	void RegisterSpawnPoint(AMonsterSpawnPoint* Point);
	void UnregisterSpawnPoint(const AMonsterSpawnPoint* Point);

	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	bool TeleportMonsterToSection(int32 Section, bool bFurthestFromPlayer = false);

	// No monster is active until this call. The first key calls it. The monster then spawns in the player section, at the point furthest from the player
	UFUNCTION(BlueprintCallable, Category = "CotS|Director")
	void ActivateMonsters();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	bool AreMonstersActive() const { return bMonstersActive; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Director")
	float GetNearestMonsterDistance(FVector Location) const;

	bool GetFurthestSpawnLocation(const FVector& From, FVector& OutLocation) const;

	void RegisterMonster(AMonsterCharacter* Monster);
	void UnregisterMonster(const AMonsterCharacter* Monster);

	// Spawned at a spawn point when the level has no monster.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director")
	TSoftClassPtr<AMonsterCharacter> MonsterClass{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float CloseDistance{1200.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float PressureRisePerSecond{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float PressureFallPerSecond{5.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "1"))
	float MaxPressure{100.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float MinHintStrength{0.1f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float HintMergeRadius{600.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float HintResolveRadius{300.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float MaxHintStrength{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float HintLifetime{20.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float SightDistance{1800.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float DetectionLingerSeconds{4.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float RetreatEndPressure{30.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float AttackPressureFraction{0.6f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float StalkPressureFraction{0.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float StalkEndPressureFraction{0.4f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Director", meta = (ClampMin = "0"))
	float AttackCooldownSeconds{30.f};

private:
	int32 PlayerSection{INDEX_NONE};
	bool bMonstersActive{};
	float Pressure{};
	bool bRetreating{};
	double NextAttackTime{};
	int32 LastPressureStage{};
	bool bEntryPending{};

	void ApplySectionEntry();
	bool bPlayerDetected{};
	double LastDetectedTime{-1.0e9};

	bool IsPlayerHidden() const;
	bool CanAMonsterSeePlayer(const APawn& Player) const;

	struct FActiveHint
	{
		FVector Location{FVector::ZeroVector};
		int32 Section{INDEX_NONE};
		float Strength{};
		double LastTime{};
		FName Source{NAME_None};
	};

	TArray<FActiveHint> Hints;

	struct FRecentSound
	{
		FVector Location{FVector::ZeroVector};
		float Strength{};
		double Time{};
		FName Source{NAME_None};
		bool bAccepted{};
	};

	TArray<FRecentSound> Sounds;

	float StatusLogTimer{};

	void DrawDebug() const;

	TArray<TWeakObjectPtr<AMonsterCharacter>> Monsters;
	TArray<TWeakObjectPtr<ASectionVolume>> SectionVolumes;
	TArray<TWeakObjectPtr<AMonsterSpawnPoint>> SpawnPoints;
};
