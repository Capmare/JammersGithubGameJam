#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "ThrowablePickup.generated.h"

class UStaticMesh;
class USphereComponent;
class UStaticMeshComponent;

/**
 * An object the player can pick up and throw.
 *
 * Put it in the level, or make a Blueprint child and set the mesh.
 * The player must look at it and press the interact key.
 * The player attaches it to the hand socket while the player holds it. The player then throws it.
 * When it hits something hard, it sends a hint to the monster director.
 */
UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API AThrowablePickup : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AThrowablePickup();

	virtual void PickUp_Implementation(AActor* Interactor) override;

	virtual void BeginPlay() override;

	virtual bool ShouldBeSaved() const override { return false; }

	// Copies the mesh of a held object. The copy then keeps that mesh and skips the random pick
	void UseMesh(UStaticMesh* NewMesh);

	// Turns physics on and sets the speed. Call it after the actor finished spawning
	void Launch(const FVector& Velocity);

	// Marks the object as thrown. Its first hit always sends a hint
	void MarkThrown() { bThrownPending = true; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	UStaticMesh* GetCurrentMesh() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsHeld() const { return bHeld; }

protected:
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** The sphere holds the physics. It is the root of the actor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> PhysicsBody{};

	/** Only shows the object. It has no collision. Move it to match the sphere. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh{};

	/** Radius of the physics sphere. Make it about the size of the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Physics", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	float CollisionRadius{25.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Physics", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float MassKg{1.f};

	/** The object picks one of these meshes at random when it spawns. Leave empty to keep the mesh on the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Visual", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMesh>> RandomMeshes{};

	/** How much speed the object keeps after a bounce. 0 does not bounce. 1 keeps all speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Physics", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float Bounciness{0.4f};

	/** Surface friction of the object. A low value slides more. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Physics", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float Friction{0.6f};

	/** Only a pawn the player controls can pick this up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	bool bPlayerOnly{true};

	/** Strength of the hint that a hard impact sends to the monster. Use 0 for no hint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float ImpactHintStrength{1.f};

	/** The first hit after a throw always makes a hint. Later bounces must be at least this strong. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float MinImpactImpulse{20000.f};

	/** Seconds between two hints from later bounces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float HintCooldown{1.f};

private:
	bool bHeld{};
	bool bThrownPending{};
	bool bMeshFixed{};
	double LastHintTime{-1000.0};
};
