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
	if (bCollected || !Interactor) return;

	if (bPlayerOnly)
	{
		const APawn* InteractingPawn{Cast<APawn>(Interactor)};
		if (!InteractingPawn || !InteractingPawn->IsPlayerControlled()) return;
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
	return true;
}
