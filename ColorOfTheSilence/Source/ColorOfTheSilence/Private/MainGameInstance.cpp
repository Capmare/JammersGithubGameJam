


#include "MainGameInstance.h"
#include "ThemeData.h"
#include "BaseProceduralActor.h"
#include "Kismet/GameplayStatics.h"
#include "MainSaveGame.h"
#include "PlayerCharacterInterface.h"
#include "InteractableInterface.h"
#include "GameFramework/Character.h"
#include "MonsterCharacter.h"
#include "CotSObjectiveSubsystem.h"


UMainGameInstance::UMainGameInstance()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> BP_ThemeData(TEXT("DataTable'/Game/Data/DataTables/DT_ThemeData.DT_ThemeData'"));
	ThemeDataTable = BP_ThemeData.Object;
}

UDataTable* UMainGameInstance::GetThemeDataTable()
{
	return ThemeDataTable;
}

void UMainGameInstance::RandomizeTheme(int32 Room)
{
	TArray<FName> RowNames = ThemeDataTable->GetRowNames();
	int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
	FName RandomRowName = RowNames[RandomIndex];
	FThemeData* RandomRowData = ThemeDataTable->FindRow<FThemeData>(RandomRowName, TEXT(""));
	if (RandomRowData)
	{
		TArray<AActor*> ProceduralActors; 
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseProceduralActor::StaticClass(), ProceduralActors);
		for (AActor* Actor : ProceduralActors)
		{
			ABaseProceduralActor* ProceduralActor = Cast<ABaseProceduralActor>(Actor);
			if (ProceduralActor && ProceduralActor->Room == Room)
			{
				ProceduralActor->GenerateRandomMesh(RandomRowData->ThemeName);
			}
		}
	}
}

void UMainGameInstance::SaveGame(FString SaveSlotName)
{
	if (UMainSaveGame* SaveGameInstance = Cast<UMainSaveGame>(UGameplayStatics::CreateSaveGameObject(UMainSaveGame::StaticClass())))
	{

		SaveGameInstance->CurrentlyHeldObjectClass = TSubclassOf<AActor>(Cast<IPlayerCharacterInterface>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))->GetHeldItem()->GetClass());

		TArray<AActor*> InteractableActors;
		UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UInteractableInterface::StaticClass(), InteractableActors);

		for (AActor* Actor : InteractableActors)
		{
			if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(Actor))
			{
				if (Interactable->ShouldBeSaved())
				{
					FInteractableObjectData ObjectData;
					ObjectData.ObjectClass = Actor->GetClass();
					ObjectData.Location = Actor->GetActorLocation();
					ObjectData.Rotation = Actor->GetActorRotation();
					SaveGameInstance->InteractableObjects.Add(ObjectData);
				}
			}
		}

		SaveGameInstance->MonsterLocation = UGameplayStatics::GetActorOfClass(GetWorld(), AMonsterCharacter::StaticClass())->GetActorLocation();
		SaveGameInstance->CollectedKeys = Cast<UCotSObjectiveSubsystem>(GetSubsystem<UCotSObjectiveSubsystem>())->GetCollectedKeyCount();

		UGameplayStatics::AsyncSaveGameToSlot(SaveGameInstance, SaveSlotName, 0);
	}
}

void UMainGameInstance::LoadGame(FString SaveSlotName)
{
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	LoadedDelegate.BindUObject(this, &UMainGameInstance::OnGameLoaded);
	UGameplayStatics::AsyncLoadGameFromSlot(SaveSlotName, 0, LoadedDelegate);
}

void UMainGameInstance::OnGameLoaded(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
	UMainSaveGame* LoadedSaveGame = Cast<UMainSaveGame>(LoadedGameData);

	TArray<AActor*> InteractableActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UInteractableInterface::StaticClass(), InteractableActors);

	for (AActor* Actor : InteractableActors)
	{
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(Actor))
		{
			if (Interactable->ShouldBeSaved())
			{
				Actor->Destroy();
			}
		}
	}

	for (const FInteractableObjectData& ObjectData : LoadedSaveGame->InteractableObjects)
	{
		if (ObjectData.ObjectClass)
		{
			FActorSpawnParameters SpawnParams;
			GetWorld()->SpawnActor<AActor>(ObjectData.ObjectClass, ObjectData.Location, ObjectData.Rotation, SpawnParams);
		}
	}

	if (LoadedSaveGame->CurrentlyHeldObjectClass)
	{
		FActorSpawnParameters SpawnParams;
		AActor* SpawnedHeldObject = GetWorld()->SpawnActor<AActor>(LoadedSaveGame->CurrentlyHeldObjectClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		Cast<IPlayerCharacterInterface>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))->PickUp(SpawnedHeldObject);
	}

	if (AMonsterCharacter* Monster = Cast<AMonsterCharacter>(UGameplayStatics::GetActorOfClass(GetWorld(), AMonsterCharacter::StaticClass())))
	{
		Monster->SetActorLocation(LoadedSaveGame->MonsterLocation);
	}
	
	if (UCotSObjectiveSubsystem* ObjectiveSubsystem = Cast<UCotSObjectiveSubsystem>(GetSubsystem<UCotSObjectiveSubsystem>()))
	{
		ObjectiveSubsystem->SetCollectedKeyCount(LoadedSaveGame->CollectedKeys);
	}

}

