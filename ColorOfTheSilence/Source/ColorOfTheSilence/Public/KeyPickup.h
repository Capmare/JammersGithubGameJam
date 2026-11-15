// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "KeyPickup.generated.h"

class AKeySpot;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * A key the player can pick up.
 *
 * Do not place these in the level. Put AKeySpot actors in the level instead.
 * The objective subsystem spawns this actor on a random spot.
 * You may place one by hand if you want a fixed key.
 *
 * The player must look at the key and press the interact key. Walking into it
 * does nothing. APlayerCharacter::TryPickUp finds this actor with a line trace
 * and calls PickUp on it. The key then tells the objective subsystem.
 */
UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API AKeyPickup : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AKeyPickup();

	// Interface call from the player interact key.
	virtual void PickUp_Implementation(AActor* Interactor) override;

	/** The spot that spawned this key. Null for a hand placed key. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	AKeySpot* GetSourceSpot() const;

	void SetSourceSpot(AKeySpot* Spot);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsCollected() const { return bCollected; }

	/**
	 * Give this key to the player.
	 * Collector is the pawn that pressed the interact key on this key.
	 * Returns false if the game is over, or if this key was already collected.
	 */
	UFUNCTION(BlueprintCallable, Category = "CotS")
	bool Collect(AActor* Collector);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> KeyRoot{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> KeyMesh{};

	/** The player interact ray hits this volume. Pawns walk through it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Interact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractVolume{};

	/**
	 * Radius of the volume the player must hit with the interact ray.
	 * Keep this small. The camera arm also casts on this channel, so a big volume can push the camera.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Interact", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float InteractRadius{50.f};

	/** Destroy the key actor when the player collects it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	bool bDestroyOnCollect{true};

	/** Only a pawn the player controls can collect the key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	bool bPlayerOnly{true};

private:
	TWeakObjectPtr<AKeySpot> SourceSpot{};
	bool bCollected{};
};
