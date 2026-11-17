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
	int32 GetSection() const { return Section; }

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

	/** Spots with the same number form one section. Each section holds at most one key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	int32 Section{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	FVector SpawnOffset{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	FRotator SpawnRotation{};

private:
	UPROPERTY()
	TObjectPtr<AKeyPickup> SpawnedKey{};
};
