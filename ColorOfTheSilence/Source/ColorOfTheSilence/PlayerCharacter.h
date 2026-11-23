// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacterInterface.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
class UPlayerInputDataAsset;
class UCameraComponent;
class USpringArmComponent;
class UPaintEffectComponent;
class UInputMappingContext;
class USceneComponent;
class USoundBase;

enum class EPlayerWalkingStatus
{
	Idle,
	Walking,
	Sprinting
};

UCLASS()
class COLOROFTHESILENCE_API APlayerCharacter : public ACharacter, public IPlayerCharacterInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual AActor* GetHeldItem() override { return HeldItem; }
	
private:
	void ApplyMovementInput(const FInputActionValue& Value);
	void ApplyLookInput(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void StartCalm();
	void StopCalm();
	void UpdateStamina(float DeltaTime);
	void UpdateHeartbeat(float DeltaTime);
	bool CanSprint() const { return !bExhausted && !bCalm; }
	void TogglePaint();
	void ApplySpeed(float NewSpeed) const;

	AActor* FindPickupCandidate();
	void TryPickUp();
	
	// IPickUpThrowInterface implementation
	virtual void PickUp(AActor* TargetActor) override;

	UPROPERTY()
	AActor* HeldItem{ nullptr };
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UPlayerInputDataAsset> InputReference{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<USpringArmComponent> CameraBoom{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UPaintEffectComponent> PaintEffect{};

	bool bPaintActive{};
	bool bSprintHeld{};
	bool bCalm{};
	bool bExhausted{};
	float Stamina{};
	float HeartbeatTimer{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UCameraComponent> PlayerCamera{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UInputMappingContext> InputMappingReference{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	float WalkingSpeed{600.f};

	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	float SprintingSpeed{900.f};

	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	float PickUpDistance{600.f};

	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	bool ShowPickUpDebugRay{false};

	// Plays once per beat. Use a single beat, not a loop.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Heartbeat")
	TObjectPtr<USoundBase> HeartbeatSound{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.05"), Category = "CotS|Heartbeat")
	float SlowestBeatInterval{1.2f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.05"), Category = "CotS|Heartbeat")
	float FastestBeatInterval{0.35f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="1"), Category = "CotS|Heartbeat")
	float MonsterFearDistance{2500.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="1"), Category = "CotS|Heartbeat")
	float CalmIntensityScale{0.3f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="1"), Category = "CotS|Stamina")
	float MaxStamina{100.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float StaminaDrainPerSecond{25.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float StaminaRegenPerSecond{15.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="1"), Category = "CotS|Stamina")
	float CalmRegenMultiplier{3.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float SprintResumeStamina{30.f};
};
