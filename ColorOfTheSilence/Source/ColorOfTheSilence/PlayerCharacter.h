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

	// Starts the sound world. The world then swaps back and forth at random times
	void BeginWorldSwapping();

	bool IsInSoundWorld() const { return bPaintActive; }

	// 0 is fully rested. 1 is fully exhausted.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Stamina")
	float GetTiredness() const { return 1.f - FMath::Clamp(Stamina / MaxStamina, 0.f, 1.f); }

	// Keeps the aim offset inside the allowed box so the spine does not twist too far
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	static FRotator ClampAimOffset(const FRotator& Offset, float MaxYaw, float MaxPitch);

	bool IsAiming() const { return bAiming; }

	// Fires when aiming starts or stops. Use it to show or hide a crosshair widget
	UFUNCTION(BlueprintImplementableEvent, Category = "CotS|Throw")
	void OnAimChanged(bool bNowAiming);
	
private:
	void ApplyMovementInput(const FInputActionValue& Value);
	void ApplyLookInput(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void UpdateCrouchCamera(float DeltaTime);
	void UpdateAimZoom(float DeltaTime);
	void PlayCrouchAnim(float Rate) const;
	void ToggleCrouch();
	void ExitCrouch();
	void StartCalm();
	void StopCalm();
	void UpdateStamina(float DeltaTime);
	void StartTiredPause();
	bool IsTiredPausing() const;
	void UpdateHeartbeat(float DeltaTime);
	bool CanSprint() const { return !bExhausted && !bCalm && !bIsCrouched; }
	void SetSoundWorld(bool bSound);
	void ScheduleNextWorldSwap();
	void ApplySpeed(float NewSpeed) const;

	void UpdateAimPreview() const;
	FVector GetThrowDirection() const;
	FVector GetThrowStart() const;

	void StartAim();
	void ThrowHeldItem();
	void CancelAim();

	AActor* FindPickupCandidate();
	void TryPickUp();
	
	// IPickUpThrowInterface implementation
	virtual void PickUp(AActor* TargetActor) override;

	UPROPERTY()
	AActor* HeldItem{ nullptr };

	bool bAiming{};
	FRotator AimBaseRotation{};
	FRotator AimOffset{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UAnimMontage> PickupAnim{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UAnimMontage> ThrowAnim{};

	// Max turn left or right, in degrees, while the player aims
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS")
	float CrouchingSpeed{300.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Crouch")
	// Stand up animation. The player plays it backward to crouch
	TObjectPtr<UAnimMontage> StandUpAnim{};

	// 1 is the montage speed. Lower is slower
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.05"), Category = "CotS|Crouch")
	float CrouchAnimRate{0.5f};

	// How fast the camera moves to the new height. Lower is slower
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.1"), Category = "CotS|Crouch")
	float CrouchCameraSpeed{3.f};

	float CameraHeightOffset{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="90"), Category = "CotS|Throw")
	float MaxAimYaw{30.f};

	// Max turn up or down, in degrees, while the player aims
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="89"), Category = "CotS|Throw")
	float MaxAimPitch{20.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Throw")
	float ThrowSpeed{1500.f};

	// Camera field of view in degrees while aiming. Lower zooms in more
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="10", ClampMax="170"), Category = "CotS|Throw")
	float AimFieldOfView{60.f};

	// Camera arm length while aiming. Lower puts the camera closer
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Throw")
	float AimArmLength{150.f};

	// How fast the zoom moves. Lower is slower
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.1"), Category = "CotS|Throw")
	float AimZoomSpeed{8.f};

	float DefaultFieldOfView{90.f};
	float DefaultArmLength{250.f};

	// Distance in front of the player where the item appears
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Throw")
	float ThrowSpawnDistance{100.f};

	// Socket on the player mesh that holds the item
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Throw")
	FName HandSocketName{TEXT("hand_r")};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Throw")
	bool bShowThrowPreview{true};

	// Seconds of flight that the preview arc shows
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.1"), Category = "CotS|Throw")
	float PreviewSeconds{2.f};

	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UPlayerInputDataAsset> InputReference{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<USpringArmComponent> CameraBoom{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UPaintEffectComponent> PaintEffect{};

	bool bPaintActive{};
	bool bWorldSwapping{};
	FTimerHandle WorldSwapTimer{};
	FRandomStream SwapRandom{};

	// Shortest and longest wait, in seconds, before the next world swap
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="1"), Category = "CotS|World")
	float MinWorldSwapSeconds{20.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="1"), Category = "CotS|World")
	float MaxWorldSwapSeconds{45.f};
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

	// Radius of the sphere that looks for a pickup. A bigger value is easier to aim
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS")
	float PickUpRadius{40.f};

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

	// Plays once when the stamina reaches 0
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Stamina")
	TObjectPtr<UAnimMontage> ExhaustedAnim{};

	// Used only when no ExhaustedAnim is set. The player then stops for a random time between these two values, in seconds
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float MinExhaustedPause{1.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float MaxExhaustedPause{2.f};

	// Seconds the montage takes to blend in. A higher value is smoother
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float ExhaustedBlendInSeconds{0.4f};

	// How fast the player slows down when the stamina runs out. A lower value is smoother
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.1"), Category = "CotS|Stamina")
	float ExhaustedSlowDownSpeed{5.f};

	double TiredPauseEndTime{};
};
