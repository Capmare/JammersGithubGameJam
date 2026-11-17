// Fill out your copyright notice in the Description page of Project Settings.


#include "KeyPickup.h"

#include "CotSObjectiveSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "KeySpot.h"

AKeyPickup::AKeyPickup()
{
	KeyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("KeyRoot"));
	SetRootComponent(KeyRoot);

	KeyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeyMesh"));
	KeyMesh->SetupAttachment(KeyRoot);
	KeyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	KeyMesh->SetGenerateOverlapEvents(false);

	InteractVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractVolume"));
	InteractVolume->SetupAttachment(KeyRoot);
	InteractVolume->SetSphereRadius(InteractRadius);
	InteractVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractVolume->SetGenerateOverlapEvents(false);
	InteractVolume->SetCanEverAffectNavigation(false);
}

void AKeyPickup::BeginPlay()
{
	Super::BeginPlay();

	InteractVolume->SetSphereRadius(InteractRadius);
}

void AKeyPickup::PickUp_Implementation(AActor* Interactor)
{
	if (bCollected || !Interactor)
	{
		UE_LOG(LogCotS, Verbose, TEXT("Key '%s' ignores the interact call. Already collected: %d"), *GetName(), bCollected ? 1 : 0);
		return;
	}

	if (bPlayerOnly)
	{
		const APawn* InteractingPawn{Cast<APawn>(Interactor)};
		if (!InteractingPawn || !InteractingPawn->IsPlayerControlled())
		{
			UE_LOG(LogCotS, Verbose, TEXT("Key '%s' ignores '%s'. The caller is not a player controlled pawn."), *GetName(), *Interactor->GetName());
			return;
		}
	}

	Collect(Interactor);
}

AKeySpot* AKeyPickup::GetSourceSpot() const
{
	return SourceSpot.Get();
}

void AKeyPickup::SetSourceSpot(AKeySpot* Spot)
{
	SourceSpot = Spot;
}

bool AKeyPickup::Collect(AActor* Collector)
{
	if (!Collector)
	{
		UE_LOG(LogCotS, Warning, TEXT("Key '%s' got a null collector."), *GetName());
		return false;
	}

	if (bCollected)
	{
		UE_LOG(LogCotS, Log, TEXT("Key '%s' is already collected. '%s' cannot collect it again."), *GetName(), *Collector->GetName());
		return false;
	}

	if (bPlayerOnly)
	{
		const APawn* CollectorPawn{Cast<APawn>(Collector)};
		if (!CollectorPawn || !CollectorPawn->IsPlayerControlled())
		{
			UE_LOG(LogCotS, Log, TEXT("Key '%s' rejects '%s'. Only a player controlled pawn may collect a key."), *GetName(), *Collector->GetName());
			return false;
		}
	}

	UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)};
	if (!Subsystem)
	{
		UE_LOG(LogCotS, Warning, TEXT("Key '%s' found no objective subsystem. The key stays in the world."), *GetName());
		return false;
	}

	if (Subsystem->IsGameFinished())
	{
		UE_LOG(LogCotS, Log, TEXT("Key '%s' is locked. The game is over."), *GetName());
		return false;
	}

	if (!Subsystem->ReportKeyCollected(this))
	{
		UE_LOG(LogCotS, Warning, TEXT("Key '%s' was not counted. The objective subsystem refused the key."), *GetName());
		return false;
	}

	bCollected = true;
	UE_LOG(LogCotS, Log, TEXT("Key '%s' collected by '%s'."), *GetName(), *Collector->GetName());

	if (AKeySpot* Spot{GetSourceSpot()}; Spot && Spot->GetSpawnedKey() == this)
	{
		Spot->ClearSpawnedKey();
		UE_LOG(LogCotS, Log, TEXT("KeySpot '%s' is free again."), *Spot->GetName());
		return true;
	}

	if (bDestroyOnCollect)
	{
		Destroy();
		UE_LOG(LogCotS, Verbose, TEXT("Key '%s' destroyed itself on collect."), *GetName());
	}
	else
	{
		UE_LOG(LogCotS, Verbose, TEXT("Key '%s' stays in the world. 'Destroy On Collect' is off."), *GetName());
	}

	return true;
}
