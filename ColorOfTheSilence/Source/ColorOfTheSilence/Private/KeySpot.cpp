// Fill out your copyright notice in the Description page of Project Settings.


#include "KeySpot.h"

#include "CotSObjectiveSubsystem.h"
#include "KeyPickup.h"
#include "SectionVolume.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AKeySpot::AKeySpot()
{
	SpotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpotRoot"));
	SetRootComponent(SpotRoot);

	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(SpotRoot);
	Marker->SetHiddenInGame(true);
	Marker->SetCastShadow(false);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetGenerateOverlapEvents(false);
}

void AKeySpot::BeginPlay()
{
	Super::BeginPlay();

	ASectionVolume::AssignSectionFromVolume(*this, Section);

	if (UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)}) Subsystem->RegisterKeySpot(this);
}

void AKeySpot::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)}) Subsystem->UnregisterKeySpot(this);

	Super::EndPlay(Reason);
}

AKeyPickup* AKeySpot::SpawnKey(const TSubclassOf<AKeyPickup> FallbackClass)
{
	if (SpawnedKey)
	{
		UE_LOG(LogCotS, Verbose, TEXT("KeySpot '%s' already holds the key '%s'."), *GetName(), *SpawnedKey->GetName());
		return SpawnedKey;
	}

	const TSubclassOf<AKeyPickup> Class{PickupClass ? PickupClass : FallbackClass};
	if (!Class)
	{
		UE_LOG(LogCotS, Warning, TEXT("KeySpot '%s' has no key class. Set 'Pickup Class' on this KeySpot, or 'Default Pickup Class' on the objective subsystem."), *GetName());
		return nullptr;
	}

	UWorld* World{GetWorld()};
	if (!World)
	{
		UE_LOG(LogCotS, Warning, TEXT("KeySpot '%s' has no world. It cannot spawn a key."), *GetName());
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = nullptr;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedKey = World->SpawnActor<AKeyPickup>(Class, GetActorLocation() + SpawnOffset, GetActorRotation() + SpawnRotation, Params);
	if (!SpawnedKey)
	{
		UE_LOG(LogCotS, Warning, TEXT("KeySpot '%s' could not spawn its key."), *GetName());
		return nullptr;
	}

	SpawnedKey->SetSourceSpot(this);
	UE_LOG(LogCotS, Log, TEXT("KeySpot '%s' spawned the key '%s' with the class '%s'."), *GetName(), *SpawnedKey->GetName(), *Class->GetName());
	return SpawnedKey;
}

void AKeySpot::ClearSpawnedKey()
{
	if (!SpawnedKey) return;

	if (SpawnedKey->GetSourceSpot() == this) SpawnedKey->SetSourceSpot(nullptr);
	SpawnedKey->Destroy();
	SpawnedKey = nullptr;
}
