// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KeySpot.generated.h"

class AKeyPickup;
class USceneComponent;
class UStaticMeshComponent;


UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API AKeySpot : public AActor
{
	GENERATED_BODY()

public:
	AKeySpot();


	AKeyPickup* SpawnKey(TSubclassOf<AKeyPickup> FallbackClass = nullptr);

	void ClearSpawnedKey();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	AKeyPickup* GetSpawnedKey() const { return SpawnedKey; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool HasKey() const { return SpawnedKey != nullptr; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SpotRoot{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Marker", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Marker{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AKeyPickup> PickupClass{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true", Description = "Name of the game section. Not used by the random pick yet."))
	FName Section{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	FVector SpawnOffset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	FRotator SpawnRotation{};

private:
	UPROPERTY()
	TObjectPtr<AKeyPickup> SpawnedKey{};
};
