// Fill out your copyright notice in the Description page of Project Settings.


#include "HubKeyLock.h"

#include "CotSObjectiveSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AHubKeyLock::AHubKeyLock()
{
	LockRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LockRoot"));
	SetRootComponent(LockRoot);

	LockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LockMesh"));
	LockMesh->SetupAttachment(LockRoot);
	LockMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockMesh->SetCollisionResponseToAllChannels(ECR_Block);
	LockMesh->SetGenerateOverlapEvents(false);
}

int32 AHubKeyLock::GetSlotCount() const
{
	if (Slots > 0) return Slots;

	const UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)};
	return Subsystem ? FMath::Max(1, Subsystem->KeysRequired) : 1;
}

void AHubKeyLock::PickUp_Implementation(AActor* Interactor)
{
	if (!Interactor || IsFull())
	{
		UE_LOG(LogCotS, Log, TEXT("Hub lock '%s' rejects the key. Inserted: %d | slots: %d"), *GetName(), InsertedKeys, GetSlotCount());
		OnKeyInsertFailed.Broadcast();
		return;
	}

	UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)};
	if (!Subsystem || !Subsystem->ReportKeyPlacedInHub())
	{
		UE_LOG(LogCotS, Log, TEXT("Hub lock '%s' rejects the key. The objective subsystem refused it."), *GetName());
		OnKeyInsertFailed.Broadcast();
		return;
	}

	++InsertedKeys;
	OnKeyInserted.Broadcast(InsertedKeys, GetSlotCount());
}
