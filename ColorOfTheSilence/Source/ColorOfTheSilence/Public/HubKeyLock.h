// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "HubKeyLock.generated.h"

class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCotSKeyInserted, int32, InsertedKeys, int32, SlotCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCotSKeyInsertFailed);


UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API AHubKeyLock : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AHubKeyLock();

	virtual void PickUp_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	int32 GetInsertedKeyCount() const { return InsertedKeys; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	int32 GetSlotCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsFull() const { return InsertedKeys >= GetSlotCount(); }

	UPROPERTY(BlueprintAssignable, Category = "CotS|Events")
	FOnCotSKeyInserted OnKeyInserted;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Events")
	FOnCotSKeyInsertFailed OnKeyInsertFailed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> LockRoot{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LockMesh{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true", ClampMin = "0", Description = "Zero means: use KeysRequired from the objective subsystem."))
	int32 Slots{};

private:
	int32 InsertedKeys{};
};
