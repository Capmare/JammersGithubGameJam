// Fill out your copyright notice in the Description page of Project Settings.


#include "CotSObjectiveSubsystem.h"

#include "KeyPickup.h"
#include "KeySpot.h"
#include "Engine/World.h"
#include "TimerManager.h"

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
}

void UCotSObjectiveSubsystem::Deinitialize()
{
	if (UWorld* World{RollWorld.Get()}) World->GetTimerManager().ClearTimer(RollTimerHandle);
	RollWorld.Reset();
	RolledWorld.Reset();

	RegisteredSpots.Empty();
	ActiveSpots.Empty();

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
}

void UCotSObjectiveSubsystem::RegisterKeySpot(AKeySpot* Spot)
{
	if (!Spot) return;

	PruneDeadSpots();

	if (ContainsSpot(RegisteredSpots, Spot)) return;
	RegisteredSpots.Add(Spot);

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

void UCotSObjectiveSubsystem::SpawnKeysForRegisteredSpots()
{
	PruneDeadSpots();

	// A new seed replaces the old one. This keeps the -1 case random and a set seed repeatable.
	if (RandomSeed != UsedSeed)
	{
		UsedSeed = RandomSeed;
		SpotRandomizer.Initialize(RandomSeed >= 0 ? RandomSeed : FMath::Rand());
	}

	const int32 Budget{GetUnspawnedKeyCount()};
	RolledWorld = GetWorld();
	if (Budget <= 0) return;

	TArray<AKeySpot*> Candidates;
	Candidates.Reserve(RegisteredSpots.Num());
	for (const TWeakObjectPtr<AKeySpot>& WeakSpot : RegisteredSpots)
	{
		AKeySpot* Spot{WeakSpot.Get()};
		if (Spot && !IsSpotActive(Spot)) Candidates.Add(Spot);
	}

	// Shuffle the spots, then take the first ones. This gives a random key set.
	for (int32 Index{Candidates.Num() - 1}; Index > 0; --Index)
	{
		const int32 SwapIndex{SpotRandomizer.RandRange(0, Index)};
		Candidates.Swap(Index, SwapIndex);
	}

	const int32 Count{FMath::Min(Budget, Candidates.Num())};
	for (int32 Index{}; Index < Count; ++Index)
	{
		if (ActivateSpot(Candidates[Index])) ++KeysSpawned;
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
	if (!Key || bGameFinished) return false;

	++CollectedKeys;
	OnKeyCollected.Broadcast(CollectedKeys, KeysRequired);
	return true;
}

bool UCotSObjectiveSubsystem::ReportKeyPlacedInHub()
{
	if (bGameFinished) return false;
	if (GetHeldKeyCount() <= 0) return false;

	++PlacedKeys;
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

	SpawnKeysForRegisteredSpots();
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
