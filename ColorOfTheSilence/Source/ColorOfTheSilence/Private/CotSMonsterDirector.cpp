#include "CotSMonsterDirector.h"

#include "AIController.h"
#include "CotSObjectiveSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NavigationData.h"
#include "Navigation/PathFollowingComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "HidingSpot.h"
#include "MonsterCharacter.h"
#include "MonsterController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MonsterFreezeComponent.h"
#include "MonsterSpawnPoint.h"
#include "SectionVolume.h"

namespace
{
	constexpr double SoundDrawSeconds{3.0};
#if ENABLE_DRAW_DEBUG
	TAutoConsoleVariable<int32> CVarCotSDebug(TEXT("cots.debug"), 1, TEXT("Draws the hints, sounds, path and target of the monster. 0 turns it off."), ECVF_Cheat);
#endif
}

UCotSMonsterDirector::UCotSMonsterDirector()
{
	MonsterClass = TSoftClassPtr<AMonsterCharacter>(FSoftObjectPath(TEXT("/Game/Shared/BP/BP_MonsterCharacter.BP_MonsterCharacter_C")));
}

UCotSMonsterDirector* UCotSMonsterDirector::Get(const UObject* WorldContextObject)
{
	const UWorld* World{WorldContextObject ? WorldContextObject->GetWorld() : nullptr};
	return World ? World->GetSubsystem<UCotSMonsterDirector>() : nullptr;
}

void UCotSMonsterDirector::SendHint(const UObject* WorldContextObject, const FVector Location, const float Strength, const FName Source)
{
	if (UCotSMonsterDirector* Director{Get(WorldContextObject)}) Director->GiveHint(Location, Strength, Source);
}

bool UCotSMonsterDirector::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World{Cast<UWorld>(Outer)};
	return World && World->IsGameWorld();
}

void UCotSMonsterDirector::SetPlayerSection(const int32 Section)
{
	if (PlayerSection == Section) return;

	UE_LOG(LogCotS, Log, TEXT("Director: the player enters section %d (was %d)."), Section, PlayerSection);
	PlayerSection = Section;

	bEntryPending = true;
}

void UCotSMonsterDirector::ApplySectionEntry()
{
	if (PlayerSection == INDEX_NONE)
	{
		for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
		{
			if (AMonsterCharacter* Monster{WeakMonster.Get()}) Monster->SetDormant(true);
		}
		return;
	}

	TeleportMonsterToSection(PlayerSection);
	for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
	{
		if (AMonsterCharacter* Monster{WeakMonster.Get()}) Monster->SetDormant(false);
	}
}

void UCotSMonsterDirector::RegisterSpawnPoint(AMonsterSpawnPoint* Point)
{
	if (Point) SpawnPoints.AddUnique(Point);
}

void UCotSMonsterDirector::UnregisterSpawnPoint(const AMonsterSpawnPoint* Point)
{
	SpawnPoints.RemoveAll([Point](const TWeakObjectPtr<AMonsterSpawnPoint>& Weak) { return !Weak.IsValid() || Weak.Get() == Point; });
}

bool UCotSMonsterDirector::TeleportMonsterToSection(const int32 Section)
{
	TArray<const AMonsterSpawnPoint*> Candidates;
	for (const TWeakObjectPtr<AMonsterSpawnPoint>& WeakPoint : SpawnPoints)
	{
		if (const AMonsterSpawnPoint* Point{WeakPoint.Get()}; Point && Point->GetSection() == Section) Candidates.Add(Point);
	}
	if (Candidates.IsEmpty())
	{
		FString Known;
		for (const TWeakObjectPtr<AMonsterSpawnPoint>& WeakPoint : SpawnPoints)
		{
			if (const AMonsterSpawnPoint* Point{WeakPoint.Get()}) Known += FString::Printf(TEXT(" %s=%d"), *Point->GetName(), Point->GetSection());
		}
		UE_LOG(LogCotS, Warning, TEXT("Director: section %d has no monster spawn point. The monster does not teleport. Known points:%s"), Section, *Known);
		return false;
	}

	if (!Monsters.ContainsByPredicate([](const TWeakObjectPtr<AMonsterCharacter>& Weak) { return Weak.IsValid(); }))
	{
		UClass* Class{MonsterClass.LoadSynchronous()};
		if (!Class)
		{
			UE_LOG(LogCotS, Warning, TEXT("Director: the level has no monster and MonsterClass did not load. Nothing spawns in section %d."), Section);
			return false;
		}

		const AMonsterSpawnPoint* Point{Candidates[FMath::RandRange(0, Candidates.Num() - 1)]};
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const AMonsterCharacter* Spawned{GetWorld()->SpawnActor<AMonsterCharacter>(Class, Point->GetActorLocation(), Point->GetActorRotation(), Params)};
		UE_LOG(LogCotS, Log, TEXT("Director: %s a monster in section %d."), Spawned ? TEXT("spawned") : TEXT("failed to spawn"), Section);
		return Spawned != nullptr;
	}

	bool bMoved{};
	for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
	{
		AMonsterCharacter* Monster{WeakMonster.Get()};
		if (!Monster) continue;

		const AMonsterSpawnPoint* Chosen{Candidates[FMath::RandRange(0, Candidates.Num() - 1)]};
		if (!Monster->TeleportTo(Chosen->GetActorLocation(), Chosen->GetActorRotation())) continue;

		const bool bWasDormant{Monster->IsDormant()};
		Monster->SetDormant(false);
		if (!bWasDormant)
		{
			if (AMonsterController* Controller{Cast<AMonsterController>(Monster->GetController())}) Controller->ResetBehavior();
		}
		bMoved = true;
		UE_LOG(LogCotS, Log, TEXT("Director: the monster teleports to a spawn point in section %d."), Section);
	}
	return bMoved;
}

float UCotSMonsterDirector::GetNearestMonsterDistance(const FVector Location) const
{
	double Best{TNumericLimits<double>::Max()};
	for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
	{
		if (const AMonsterCharacter* Monster{WeakMonster.Get()}; Monster && !Monster->IsDormant()) Best = FMath::Min(Best, FVector::Dist(Monster->GetActorLocation(), Location));
	}
	return static_cast<float>(Best);
}

void UCotSMonsterDirector::RegisterMonster(AMonsterCharacter* Monster)
{
	if (Monster) Monsters.AddUnique(Monster);
}

void UCotSMonsterDirector::UnregisterMonster(const AMonsterCharacter* Monster)
{
	Monsters.RemoveAll([Monster](const TWeakObjectPtr<AMonsterCharacter>& Weak) { return !Weak.IsValid() || Weak.Get() == Monster; });
}

void UCotSMonsterDirector::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APawn* Player{UGameplayStatics::GetPlayerPawn(this, 0)};
	if (!Player) return;

	// A section volume can set the section in its BeginPlay, before the monsters and spawn points register.
	if (bEntryPending)
	{
		bEntryPending = false;
		ApplySectionEntry();
	}

	// A section volume misses a player who spawns inside it: the pawn is not player controlled yet.
	if (PlayerSection == INDEX_NONE)
	{
		const int32 Found{ASectionVolume::FindSectionAt(GetWorld(), Player->GetActorLocation())};
		if (Found != INDEX_NONE) SetPlayerSection(Found);
		else
		{
			for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
			{
				if (AMonsterCharacter* Monster{WeakMonster.Get()}) Monster->SetDormant(true);
			}
		}
	}

	bool bClose{};
	for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
	{
		const AMonsterCharacter* Monster{WeakMonster.Get()};
		if (Monster && !Monster->IsDormant() && FVector::Dist(Monster->GetActorLocation(), Player->GetActorLocation()) <= CloseDistance)
		{
			bClose = true;
			break;
		}
	}

	const double Now{GetWorld()->GetTimeSeconds()};
	Sounds.RemoveAll([Now](const FRecentSound& Sound) { return Now - Sound.Time > SoundDrawSeconds; });
	Hints.RemoveAll([this, Now](const FActiveHint& Hint)
	{
		if (Now - Hint.LastTime > HintLifetime) return true;

		for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
		{
			const AMonsterCharacter* Monster{WeakMonster.Get()};
			if (Monster && !Monster->IsDormant() && FVector::Dist(Monster->GetActorLocation(), Hint.Location) <= HintResolveRadius)
			{
				UE_LOG(LogCotS, Log, TEXT("Director: the monster resolves a hint in section %d (strength %.2f)."), Hint.Section, Hint.Strength);
				return true;
			}
		}
		return false;
	});

	Pressure = FMath::Clamp(Pressure + (bClose ? PressureRisePerSecond : -PressureFallPerSecond) * DeltaTime, 0.f, MaxPressure);

	if (!bRetreating && Pressure >= MaxPressure)
	{
		bRetreating = true;
		UE_LOG(LogCotS, Log, TEXT("Director: pressure is full. The monster retreats."));
	}
	else if (bRetreating && Pressure <= RetreatEndPressure)
	{
		bRetreating = false;
		UE_LOG(LogCotS, Log, TEXT("Director: the retreat ends."));
	}

	const double DetectionNow{GetWorld()->GetTimeSeconds()};
	if (CanAMonsterSeePlayer(*Player)) LastDetectedTime = DetectionNow;

	const bool bDetected{DetectionNow - LastDetectedTime <= DetectionLingerSeconds};
	if (bDetected != bPlayerDetected)
	{
		bPlayerDetected = bDetected;
		UE_LOG(LogCotS, Log, TEXT("Director: the monster %s the player."), bPlayerDetected ? TEXT("found") : TEXT("lost"));
	}

	StatusLogTimer -= DeltaTime;
	if (StatusLogTimer <= 0.f)
	{
		StatusLogTimer = 1.f;
		const APlayerCameraManager* Camera{UGameplayStatics::GetPlayerCameraManager(this, 0)};
		for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
		{
			if (const AMonsterCharacter* Monster{WeakMonster.Get()})
			{
				UE_LOG(LogCotS, Verbose, TEXT("Director: monster at %s | speed %.0f | %s | camera %s facing %s | player section %d | pressure %.0f | hints %d | %s"), *Monster->GetActorLocation().ToString(), Monster->GetVelocity().Size(), Monster->GetFreezeComponent() && Monster->GetFreezeComponent()->IsSeenByPlayer() ? TEXT("SEEN") : TEXT("unseen"), Camera ? *Camera->GetCameraLocation().ToString() : TEXT("none"), Camera ? *Camera->GetCameraRotation().Vector().ToString() : TEXT("none"), PlayerSection, Pressure, Hints.Num(), bRetreating ? TEXT("retreating") : TEXT("hunting"));
			}
		}
	}

	DrawDebug();
}

void UCotSMonsterDirector::NotifyPlayerFound()
{
	LastDetectedTime = GetWorld()->GetTimeSeconds();
}

bool UCotSMonsterDirector::IsPlayerHidden() const
{
	const UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(this)};
	if (!Objectives) return false;

	for (const int32 Section : Objectives->GetHidingSpotSections())
	{
		for (const AHidingSpot* Spot : Objectives->GetHidingSpotsInSection(Section))
		{
			if (Spot->IsOccupiedByPlayer()) return true;
		}
	}
	return false;
}

bool UCotSMonsterDirector::CanAMonsterSeePlayer(const APawn& Player) const
{
	if (IsPlayerHidden()) return false;

	for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
	{
		const AMonsterCharacter* Monster{WeakMonster.Get()};
		if (!Monster || Monster->IsDormant()) continue;

		const FVector Eye{Monster->GetActorLocation() + FVector(0.f, 0.f, 60.f)};
		if (FVector::Dist(Eye, Player.GetActorLocation()) > SightDistance) continue;

		FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterDetect), false);
		Params.AddIgnoredActor(Monster);
		Params.AddIgnoredActor(&Player);
		if (!GetWorld()->LineTraceTestByChannel(Eye, Player.GetActorLocation(), ECC_Visibility, Params)) return true;
	}
	return false;
}

void UCotSMonsterDirector::RegisterSectionVolume(ASectionVolume* Volume)
{
	if (Volume) SectionVolumes.AddUnique(Volume);
}

void UCotSMonsterDirector::UnregisterSectionVolume(const ASectionVolume* Volume)
{
	SectionVolumes.RemoveAll([Volume](const TWeakObjectPtr<ASectionVolume>& Weak) { return !Weak.IsValid() || Weak.Get() == Volume; });
}

int32 UCotSMonsterDirector::GetSectionAtLocation(const FVector Location) const
{
	if (const int32 Found{ASectionVolume::FindSectionAt(GetWorld(), Location)}; Found != INDEX_NONE) return Found;

	const UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(this)};
	if (!Objectives) return INDEX_NONE;

	int32 BestSection{INDEX_NONE};
	double BestDistance{TNumericLimits<double>::Max()};
	for (const int32 Section : Objectives->GetHidingSpotSections())
	{
		for (const AHidingSpot* Spot : Objectives->GetHidingSpotsInSection(Section))
		{
			const double Distance{FVector::DistSquared(Spot->GetActorLocation(), Location)};
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				BestSection = Section;
			}
		}
	}
	return BestSection;
}

void UCotSMonsterDirector::GiveHint(const FVector Location, const float Strength, const FName Source)
{
	const double Now{GetWorld()->GetTimeSeconds()};
	Sounds.Add({Location, Strength, Now, Source, Strength >= MinHintStrength});

	if (Strength < MinHintStrength) return;

	FActiveHint* Near{};
	for (FActiveHint& Hint : Hints)
	{
		if (FVector::Dist(Hint.Location, Location) <= HintMergeRadius)
		{
			Near = &Hint;
			break;
		}
	}

	if (Near)
	{
		const float Total{Near->Strength + Strength};
		Near->Location = (Near->Location * Near->Strength + Location * Strength) / Total;
		Near->Strength = FMath::Min(Total, MaxHintStrength);
		Near->LastTime = Now;
		UE_LOG(LogCotS, Log, TEXT("Director: hint '%s' merged | section %d | strength %.2f"), *Source.ToString(), Near->Section, Near->Strength);
		return;
	}

	const int32 Section{GetSectionAtLocation(Location)};
	if (Section == INDEX_NONE)
	{
		UE_LOG(LogCotS, Warning, TEXT("Director: the hint '%s' is ignored. No section holds %s."), *Source.ToString(), *Location.ToString());
		return;
	}

	Hints.Add({Location, Section, FMath::Min(Strength, MaxHintStrength), Now, Source});
	UE_LOG(LogCotS, Log, TEXT("Director: hint '%s' added | section %d | strength %.2f | hints: %d"), *Source.ToString(), Section, Strength, Hints.Num());
}

int32 UCotSMonsterDirector::DropHintsNear(const FVector Location)
{
	const int32 Removed{Hints.RemoveAll([this, &Location](const FActiveHint& Hint) { return FVector::Dist(Hint.Location, Location) <= HintMergeRadius; })};
	if (Removed > 0) UE_LOG(LogCotS, Log, TEXT("Director: dropped %d hint(s) near %s."), Removed, *Location.ToString());
	return Removed;
}

void UCotSMonsterDirector::ReportNoise(const FVector Location, const float Loudness)
{
	GiveHint(Location, Loudness, TEXT("Noise"));
}

void UCotSMonsterDirector::GetHint(int32& OutSection, bool& bOutRetreat, bool& bOutHasLocation, FVector& OutLocation, float& OutStrength)
{
	bOutRetreat = bRetreating;
	bOutHasLocation = false;
	OutLocation = FVector::ZeroVector;
	OutStrength = 0.f;
	OutSection = PlayerSection;

	if (bRetreating)
	{
		const UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(this)};
		if (!Objectives) return;

		TArray<int32> Others{Objectives->GetHidingSpotSections()};
		Others.Remove(PlayerSection);
		if (!Others.IsEmpty()) OutSection = Others[FMath::RandRange(0, Others.Num() - 1)];
		return;
	}

	const FActiveHint* Strongest{};
	for (const FActiveHint& Hint : Hints)
	{
		if (!Strongest || Hint.Strength > Strongest->Strength) Strongest = &Hint;
	}

	if (Strongest)
	{
		OutSection = Strongest->Section;
		bOutHasLocation = true;
		OutLocation = Strongest->Location;
		OutStrength = Strongest->Strength;
	}
}

void UCotSMonsterDirector::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (CVarCotSDebug.GetValueOnGameThread() == 0) return;

	const UWorld* World{GetWorld()};
	const double Now{World->GetTimeSeconds()};
	const FVector Lift{0.f, 0.f, 40.f};

	for (const FRecentSound& Sound : Sounds)
	{
		const float Age{static_cast<float>((Now - Sound.Time) / SoundDrawSeconds)};
		const FColor Color{Sound.bAccepted ? FColor::Orange : FColor(110, 110, 110)};
		const float Radius{60.f + 60.f * Sound.Strength + 300.f * Age};
		DrawDebugSphere(World, Sound.Location, Radius, 16, Color, false, 0.f, 0, 2.f);
		DrawDebugString(World, Sound.Location + FVector(0.f, 0.f, Radius), FString::Printf(TEXT("%s %.1f%s"), *Sound.Source.ToString(), Sound.Strength, Sound.bAccepted ? TEXT("") : TEXT(" (too weak)")), nullptr, Color, 0.f, true);
	}

	const FActiveHint* Strongest{};
	for (const FActiveHint& Hint : Hints)
	{
		if (!Strongest || Hint.Strength > Strongest->Strength) Strongest = &Hint;

		const double Left{HintLifetime - (Now - Hint.LastTime)};
		DrawDebugSphere(World, Hint.Location, HintResolveRadius, 16, FColor::Green, false, 0.f, 0, 3.f);
		DrawDebugSphere(World, Hint.Location, HintMergeRadius, 16, FColor(0, 90, 0), false, 0.f, 0, 1.f);
		DrawDebugSphere(World, Hint.Location, 20.f + 15.f * Hint.Strength, 12, FColor::Yellow, false, 0.f, 0, 2.f);
		DrawDebugString(World, Hint.Location + FVector(0.f, 0.f, 160.f), FString::Printf(TEXT("Hint '%s' | section %d | strength %.1f | %.0f s left"), *Hint.Source.ToString(), Hint.Section, Hint.Strength, Left), nullptr, FColor::White, 0.f, true);
	}

	for (const TWeakObjectPtr<AMonsterCharacter>& WeakMonster : Monsters)
	{
		const AMonsterCharacter* Monster{WeakMonster.Get()};
		if (!Monster || Monster->IsDormant()) continue;

		const FVector Position{Monster->GetActorLocation()};
		DrawDebugCircle(World, Position - FVector(0.f, 0.f, 80.f), CloseDistance, 48, FColor::Red, false, 0.f, 0, 2.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

		if (Strongest && !bRetreating) DrawDebugLine(World, Position + Lift, Strongest->Location + Lift, FColor::Yellow, false, 0.f, 0, 2.f);

		if (bPlayerDetected)
		{
			if (const APawn* Player{UGameplayStatics::GetPlayerPawn(this, 0)})
			{
				DrawDebugLine(World, Position + Lift, Player->GetActorLocation(), FColor::Red, false, 0.f, 0, 6.f);
				DrawDebugString(World, Player->GetActorLocation() + FVector(0.f, 0.f, 120.f), TEXT("FOUND"), nullptr, FColor::Red, 0.f, true, 2.f);
			}
		}

		FString PathText{TEXT("no path")};
		const AAIController* Controller{Cast<AAIController>(Monster->GetController())};
		const UPathFollowingComponent* Follower{Controller ? Controller->GetPathFollowingComponent() : nullptr};
		if (Follower)
		{
			PathText = Follower->GetStatusDesc();
			const FNavPathSharedPtr& Path{Follower->GetPath()};
			if (Path.IsValid() && Path->GetPathPoints().Num() > 1)
			{
				const TArray<FNavPathPoint>& Points{Path->GetPathPoints()};
				for (int32 Index{1}; Index < Points.Num(); ++Index)
				{
					DrawDebugLine(World, Points[Index - 1].Location + Lift, Points[Index].Location + Lift, FColor::Cyan, false, 0.f, 0, 3.f);
					DrawDebugSphere(World, Points[Index].Location + Lift, 25.f, 8, FColor::Cyan, false, 0.f, 0, 2.f);
				}
				DrawDebugSphere(World, Points.Last().Location + Lift, 70.f, 12, FColor::Red, false, 0.f, 0, 4.f);
				DrawDebugString(World, Points.Last().Location + FVector(0.f, 0.f, 140.f), TEXT("Destination"), nullptr, FColor::Red, 0.f, true);
				DrawDebugLine(World, Position + Lift, Follower->GetCurrentTargetLocation() + Lift, FColor::Magenta, false, 0.f, 0, 4.f);
			}
		}

		const UMonsterFreezeComponent* Freeze{Monster->GetFreezeComponent()};
		const TCHAR* Motion{Freeze && Freeze->IsFrozen() ? TEXT("frozen") : TEXT("moving")};
		DrawDebugString(World, Position + FVector(0.f, 0.f, 140.f), FString::Printf(TEXT("%s | %s | pressure %.0f/%.0f%s | player section %d"), Motion, *PathText, Pressure, MaxPressure, bRetreating ? TEXT(" RETREAT") : TEXT(""), PlayerSection), nullptr, FColor::White, 0.f, true);
	}
#endif
}
