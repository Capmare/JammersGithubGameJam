// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MainSaveGame.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FInteractableObjectData
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AActor> ObjectClass{};

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;
};

UCLASS()
class COLOROFTHESILENCE_API UMainSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:

	UPROPERTY(VisibleAnywhere, Category = Monster)
	FVector MonsterLocation{};

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	TSubclassOf<AActor> CurrentlyHeldObjectClass{};

	UPROPERTY(VisibleAnywhere, Category = Interaction)
	TArray<FInteractableObjectData> InteractableObjects{};

	UPROPERTY(VisibleAnywhere, Category = Keys)
	int32 CollectedKeys{};

};
