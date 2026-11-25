// Fill out your copyright notice in the Description page of Project Settings.


#include "CotSObjectiveSubsystem.h"

#include "CotSMonsterDirector.h"
#include "HidingSpot.h"
#include "KeyPickup.h"
#include "KeySpot.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogCotS);

UCotSObjectiveSubsystem* UCotSObjectiveSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	const UWorld* World{WorldContextObject->GetWorld()};
	const UGameInstance* GameInstance{World ? World->GetGameInstance() : nullptr};
	return GameInstance ? GameInstance->GetSubsystem<UCotSObjectiveSubsystem>() : nullptr;
}

void UCotSObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogCotS, Log, TEXT("Objective subsystem is ready. Keys required: %d | default key class: %s"),
		KeysRequired, DefaultPickupClass ? *DefaultPickupClass->GetName() : TEXT("None"));
}

void UCotSObjectiveSubsystem::Deinitialize()
{
	if (UWorld* World{RollWorld.Get()}) World->GetTimerManager().ClearTimer(RollTimerHandle);
	RollWorld.Reset();
	RolledWorld.Reset();
	bRandomizerSeeded = false;

	RegisteredSpots.Empty();
	ActiveSpots.Empty();
	HidingSpots.Empty();

	Super::Deinitialize();
}

void UCotSObjectiveSubsystem::ScheduleRoll(UWorld* World)
{
	if (!World || !World->IsGameWorld()) return;

	if (RollWorld.Get() == World && RollTimerHandle.IsValid()) return;

	FTimerManager& TimerManager{World->GetTimerManager()};
	if (RollWorld.Get() == World) TimerManager.ClearTimer(RollTimerHandle);

	RollWorld = World;
	RollTimerHandle = TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCotSObjectiveSubsystem::SpawnKeysForRegisteredSpots));
	UE_LOG(LogCotS, Verbose, TEXT("Key roll scheduled for the next tick in world '%s'."), *World->GetName());
}

void UCotSObjectiveSubsystem::RegisterKeySpot(AKeySpot* Spot)
{
	if (!Spot) return;

	PruneDeadSpots();

	if (ContainsSpot(RegisteredSpots, Spot)) return;
	RegisteredSpots.Add(Spot);

	UE_LOG(LogCotS, Verbose, TEXT("KeySpot '%s' registered. Registered spots: %d"), *Spot->GetName(), RegisteredSpots.Num());

	if (RolledWorld.Get() == Spot->GetWorld()) SpawnKeysForRegisteredSpots();
	else ScheduleRoll(Spot->GetWorld());
}

void UCotSObjectiveSubsystem::UnregisterKeySpot(const AKeySpot* Spot)
{
	if (!Spot) return;

	RegisteredSpots.RemoveAll([Spot](const TWeakObjectPtr<AKeySpot>& WeakSpot) { return !WeakSpot.IsValid() || WeakSpot.Get() == Spot; });
}

bool UCotSObjectiveSubsystem::IsSpotActive(const AKeySpot* Spot) const
{
	return ContainsSpot(ActiveSpots, Spot);
}

void UCotSObjectiveSubsystem::RegisterHidingSpot(AHidingSpot* Spot)
{
	if (!Spot) return;

	HidingSpots.RemoveAll([](const TWeakObjectPtr<AHidingSpot>& WeakSpot) { return !WeakSpot.IsValid(); });
	HidingSpots.AddUnique(Spot);
	UE_LOG(LogCotS, Verbose, TEXT("HidingSpot '%s' registered in section %d. Hiding spots: %d"), *Spot->GetName(), Spot->GetSection(), HidingSpots.Num());
}

void UCotSObjectiveSubsystem::UnregisterHidingSpot(const AHidingSpot* Spot)
{
	HidingSpots.RemoveAll([Spot](const TWeakObjectPtr<AHidingSpot>& WeakSpot) { return !WeakSpot.IsValid() || WeakSpot.Get() == Spot; });
}

TArray<int32> UCotSObjectiveSubsystem::GetHidingSpotSections() const
{
	TArray<int32> Result;
	for (const TWeakObjectPtr<AHidingSpot>& WeakSpot : HidingSpots)
	{
		if (const AHidingSpot* Spot{WeakSpot.Get()}) Result.AddUnique(Spot->GetSection());
	}
	return Result;
}

TArray<AHidingSpot*> UCotSObjectiveSubsystem::GetHidingSpotsInSection(const int32 Section) const
{
	TArray<AHidingSpot*> Result;
	for (const TWeakObjectPtr<AHidingSpot>& WeakSpot : HidingSpots)
	{
		AHidingSpot* Spot{WeakSpot.Get()};
		if (Spot && Spot->GetSection() == Section) Result.Add(Spot);
	}
	return Result;
}

bool UCotSObjectiveSubsystem::IsSectionActive(const int32 Section) const
{
	return ActiveSpots.ContainsByPredicate([Section](const TWeakObjectPtr<AKeySpot>& WeakSpot)
	{
		const AKeySpot* Spot{WeakSpot.Get()};
		return Spot && Spot->GetSection() == Section;
	});
}

void UCotSObjectiveSubsystem::SpawnKeysForRegisteredSpots()
{
	PruneDeadSpots();

	// A new seed replaces the old one. This keeps the -1 case random and a set seed repeatable.
	if (NeedsReseed(bRandomizerSeeded, RandomSeed, UsedSeed))
	{
		bRandomizerSeeded = true;
		UsedSeed = RandomSeed;
		SpotRandomizer.Initialize(RandomSeed >= 0 ? RandomSeed : FMath::Rand());
	}

	const int32 Budget{GetUnspawnedKeyCount()};
	RolledWorld = GetWorld();
	if (Budget <= 0)
	{
		UE_LOG(LogCotS, Log, TEXT("Key roll skipped. Keys required: %d | keys already spawned: %d. Call Reset Objectives to spawn keys again."),
			KeysRequired, KeysSpawned);
		return;
	}

	TArray<AKeySpot*> Candidates;
	Candidates.Reserve(RegisteredSpots.Num());
	for (const TWeakObjectPtr<AKeySpot>& WeakSpot : RegisteredSpots)
	{
		AKeySpot* Spot{WeakSpot.Get()};
		if (Spot && !IsSpotActive(Spot) && !IsSectionActive(Spot->GetSection())) Candidates.Add(Spot);
	}

	if (Candidates.Num() == 0)
	{
		UE_LOG(LogCotS, Warning, TEXT("Key roll found no free KeySpot. Registered spots: %d | spots that already hold a key: %d. Place more KeySpot actors, or call Reset Objectives."),
			RegisteredSpots.Num(), ActiveSpots.Num());
		return;
	}

	// Shuffle the spots, then take the first ones. This gives a random key set.
	for (int32 Index{Candidates.Num() - 1}; Index > 0; --Index)
	{
		const int32 SwapIndex{SpotRandomizer.RandRange(0, Index)};
		Candidates.Swap(Index, SwapIndex);
	}

	// The list is shuffled, so the first spot of each section is a random spot of that section.
	TSet<int32> UsedSections;
	const int32 Count{FMath::Min(Budget, Candidates.Num())};
	int32 Spawned{};
	for (AKeySpot* Spot : Candidates)
	{
		if (Spawned >= Budget) break;
		if (UsedSections.Contains(Spot->GetSection())) continue;

		if (ActivateSpot(Spot))
		{
			UsedSections.Add(Spot->GetSection());
			++KeysSpawned;
			++Spawned;
		}
	}

	UE_LOG(LogCotS, Log, TEXT("Key roll: registered spots: %d | free spots: %d | max keys: %d | spawned: %d | total spawned: %d | keys required: %d | seed: %d"),
		RegisteredSpots.Num(), Candidates.Num(), Count, Spawned, KeysSpawned, KeysRequired, UsedSeed);

	if (Spawned < Count)
	{
		UE_LOG(LogCotS, Warning, TEXT("Key roll could only spawn %d of %d keys. Check 'Pickup Class' on the KeySpot and 'Default Pickup Class' on this subsystem."),
			Spawned, Count);
	}
}

bool UCotSObjectiveSubsystem::ActivateSpot(AKeySpot* Spot)
{
	if (!Spot) return false;

	AKeyPickup* Key{Spot->SpawnKey(DefaultPickupClass)};
	if (!Key) return false;

	ActiveSpots.Add(Spot);
	return true;
}

bool UCotSObjectiveSubsystem::ReportKeyCollected(const AKeyPickup* Key)
{
	if (!Key) return false;

	if (bGameFinished)
	{
		UE_LOG(LogCotS, Log, TEXT("Key '%s' was not counted. The game is over."), *Key->GetName());
		return false;
	}

	++CollectedKeys;
	UE_LOG(LogCotS, Log, TEXT("Key counted: %d / %d"), CollectedKeys, KeysRequired);
	if (KeyCollectedHintStrength > 0.f)
	{
		UCotSMonsterDirector::SendHint(this, Key->GetActorLocation(), KeyCollectedHintStrength, TEXT("KeyCollected"));
	}
	OnKeyCollected.Broadcast(CollectedKeys, KeysRequired);
	return true;
}

bool UCotSObjectiveSubsystem::ReportKeyPlacedInHub()
{
	if (bGameFinished) return false;
	if (GetHeldKeyCount() <= 0)
	{
		UE_LOG(LogCotS, Log, TEXT("The hub lock rejected the key. The player holds no key. Collected: %d | placed: %d"), CollectedKeys, PlacedKeys);
		return false;
	}

	++PlacedKeys;
	UE_LOG(LogCotS, Log, TEXT("Key placed in hub: %d / %d"), PlacedKeys, KeysRequired);
	if (KeyPlacedHintStrength > 0.f && bHasHubLocation)
	{
		UCotSMonsterDirector::SendHint(this, HubLocation, KeyPlacedHintStrength, TEXT("KeyPlaced"));
	}
	OnKeyPlacedInHub.Broadcast(PlacedKeys, KeysRequired);

	if (PlacedKeys >= KeysRequired)
	{
		bGameFinished = true;
		Outcome = EGameOutcome::Won;
		OnGameWon.Broadcast();
	}

	return true;
}

void UCotSObjectiveSubsystem::ReportPlayerCaught()
{
	if (bGameFinished) return;

	bGameFinished = true;
	Outcome = EGameOutcome::Lost;
	UE_LOG(LogCotS, Log, TEXT("Game over: the player was caught."));
	OnGameLost.Broadcast();
}

void UCotSObjectiveSubsystem::ResetObjectives()
{
	bGameFinished = false;
	RolledWorld.Reset();
	Outcome = EGameOutcome::None;

	KeysSpawned = 0;
	CollectedKeys = 0;
	PlacedKeys = 0;

	ActiveSpots.Empty();

	for (const TWeakObjectPtr<AKeySpot>& WeakSpot : RegisteredSpots)
	{
		if (AKeySpot* Spot{WeakSpot.Get()}) Spot->ClearSpawnedKey();
	}

	UE_LOG(LogCotS, Log, TEXT("Objectives reset. Registered spots: %d"), RegisteredSpots.Num());
	SpawnKeysForRegisteredSpots();
}

void UCotSObjectiveSubsystem::LogObjectiveState() const
{
	int32 LiveSpots{};
	for (const TWeakObjectPtr<AKeySpot>& WeakSpot : RegisteredSpots)
	{
		if (WeakSpot.IsValid()) ++LiveSpots;
	}

	UE_LOG(LogCotS, Log, TEXT("Objective state | world: %s | registered KeySpot actors: %d | spots with a key: %d | keys spawned: %d | keys required: %d | collected: %d | placed in hub: %d | held by player: %d | game finished: %d | outcome: %s | default key class: %s"),
		GetWorld() ? *GetWorld()->GetName() : TEXT("None"),
		LiveSpots,
		ActiveSpots.Num(),
		KeysSpawned,
		KeysRequired,
		CollectedKeys,
		PlacedKeys,
		GetHeldKeyCount(),
		bGameFinished ? 1 : 0,
		*UEnum::GetValueAsString(Outcome),
		DefaultPickupClass ? *DefaultPickupClass->GetName() : TEXT("None"));

	if (LiveSpots == 0)
	{
		UE_LOG(LogCotS, Warning, TEXT("No KeySpot actor registered. Place a KeySpot actor in the active level, and check that the level is loaded."));
	}
}

void UCotSObjectiveSubsystem::PruneDeadSpots()
{
	RegisteredSpots.RemoveAll([](const TWeakObjectPtr<AKeySpot>& WeakSpot) { return !WeakSpot.IsValid(); });
	ActiveSpots.RemoveAll([](const TWeakObjectPtr<AKeySpot>& WeakSpot) { return !WeakSpot.IsValid(); });
}

bool UCotSObjectiveSubsystem::ContainsSpot(const TArray<TWeakObjectPtr<AKeySpot>>& List, const AKeySpot* Spot) const
{
	if (!Spot) return false;

	return List.ContainsByPredicate([Spot](const TWeakObjectPtr<AKeySpot>& WeakSpot) { return WeakSpot.Get() == Spot; });
}
