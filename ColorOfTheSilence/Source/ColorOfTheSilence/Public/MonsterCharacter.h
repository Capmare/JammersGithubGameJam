#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterCharacter.generated.h"

class UMonsterFreezeComponent;
class UCameraComponent;
class UAnimMontage;
class APlayerCharacter;

UCLASS()
class COLOROFTHESILENCE_API AMonsterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMonsterCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsDormant() const { return bDormant; }

	// A dormant monster is hidden, has no collision and runs no State Tree.
	void SetDormant(bool bNewDormant);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	UMonsterFreezeComponent* GetFreezeComponent() const { return FreezeComponent; }

	UFUNCTION(BlueprintCallable, Category = "CotS|Grab")
	bool GrabPlayer(APlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "CotS|Grab")
	void ReleasePlayer();

	UFUNCTION(BlueprintCallable, Category = "CotS|Grab")
	bool OrderAttack(APlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Grab")
	bool IsGrabbing() const { return bGrabbing; }

	const TCHAR* GetModeLabel() const;

	bool IsFreezeSuspended() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Grab")
	bool IsStalking() const;

	UFUNCTION(BlueprintCallable, Category = "CotS|Grab")
	bool BeginStalk(APlayerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "CotS|Grab")
	void EndStalk(bool bResumeBehavior);

	bool ShouldAlwaysSpawn() const
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		return bDebugAlwaysSpawn;
#endif
	}

	UCameraComponent* GetGrabCamera() const { return GrabCamera; }
	FName GetHoldSocketName() const { return HoldSocketName; }
	FVector GetHoldLocation() const;
	const FTransform& GetPlayerHoldTransform() const { return PlayerHoldTransform; }
	float GetGrabCameraBlendTime() const { return GrabCameraBlendTime; }
	float GetEscapeDistance() const { return EscapeDistance; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterFreezeComponent> FreezeComponent{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS|Grab", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> GrabCamera{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	TObjectPtr<UAnimMontage> AttackAnim{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	TObjectPtr<UAnimMontage> HoldingPlayerAnim{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	TObjectPtr<UAnimMontage> DropAnim{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float StalkTurnSpeed{180.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float AttackLungeDistance{450.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float StalkExitSeconds{0.6f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float FleeSpeed{600.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "1"))
	float FleeTimeoutSeconds{12.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float FleeDistance{1500.f};

	// A spawn point closer than this to the player is not a valid place to flee to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float MinFleeDistance{1000.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	FName HoldSocketName{TEXT("L_Hand")};

	// The player is held at the middle point between this socket and HoldSocketName
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	FName SecondHoldSocketName{TEXT("R_Hand")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	FTransform PlayerHoldTransform{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float GrabDelaySeconds{0.6f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float StunSecondsAfterEscape{4.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float EscapeDistance{150.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float GrabCameraBlendTime{0.5f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	FName GrabCameraSocketName{TEXT("HeadSocket")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab")
	FVector GrabCameraOffset{FVector::ZeroVector};

	// Near clip plane of the grab camera, in world units. Small values let it see geometry close to the lens
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0.01"))
	float GrabCameraNearClip{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "0"))
	float AttackGrabDistance{250.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Grab", meta = (ClampMin = "1"))
	float AttackTimeoutSeconds{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CotS|Debug")
	bool bDebugAlwaysSpawn{};

	bool bDormant{};

private:
	void CompleteGrab();
	void PlayHoldLoop();
	void EndStun();
	void EndAttack(bool bResumeBehavior);
	void StartFlee();
	void UpdateGrabCamera();
	bool FindFleeLocation(FVector& OutLocation) const;
	void EndFlee();

	bool bFleeing{};
	float FleeTimeLeft{};
	float SavedWalkSpeed{};
	FVector FleeLocation{};

	bool bStalking{};
	bool bAttacking{};
	float AttackTimeLeft{};
	float AttackRepathTime{};

	bool bGrabbing{};
	double NextGrabTime{};
	TWeakObjectPtr<APlayerCharacter> GrabbedPlayer{};
	FTimerHandle GrabTimer{};
	FTimerHandle StunTimer{};
	FTimerHandle StalkExitTimer{};
};
