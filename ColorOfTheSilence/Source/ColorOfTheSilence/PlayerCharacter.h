// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "CotSDifficulty.h"
#include "GameFramework/Character.h"
#include "PlayerCharacterInterface.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
class UPlayerInputDataAsset;
class UCameraComponent;
class USpringArmComponent;
class UPaintEffectComponent;
class UCotSHeartbeatComponent;
class UCotSQTEPromptComponent;
class UCotSButtonPromptWidget;
class UUIButtonDataAsset;
class UInputMappingContext;
class USceneComponent;
class USoundBase;
class UAnimSequenceBase;
class AMonsterCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCotSPickedUp, AMonsterCharacter*, Monster);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCotSStruggleStarted, float, TimeLimit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCotSStruggleProgress, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCotSStrugglePressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCotSEscaped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCotSCaptureFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCotSQTEMissed, int32, Fails, int32, FailLimit);

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|World")
	bool IsInSoundWorld() const { return bPaintActive; }

	void BeginPickedUp(AMonsterCharacter* Monster);
	void EndPickedUp(bool bEscaped);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Grab")
	bool IsPickedUp() const { return bPickedUp; }

	// 0 is caught. 1 is escaped
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Grab")
	float GetStruggleProgress() const { return StruggleProgress; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Grab")
	float GetStruggleTimeRemaining() const { return StruggleTimeRemaining; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Grab")
	float GetStruggleTimeLimit() const { return StruggleTimeLimit; }

	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSPickedUp OnPickedUp;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSStruggleStarted OnStruggleStarted;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSStrugglePressed OnStrugglePressed;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSStruggleProgress OnStruggleProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSEscaped OnEscaped;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSCaptureFailed OnCaptureFailed;

	// Fires for each missed QTE prompt. Use it to show the misses in the UI
	UPROPERTY(BlueprintAssignable, Category = "CotS|Grab|Events")
	FOnCotSQTEMissed OnQTEMissed;

	// 0 is fully rested. 1 is fully exhausted.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Stamina")
	float GetTiredness() const { return 1.f - FMath::Clamp(Stamina / MaxStamina, 0.f, 1.f); }

	// True while the player sprints. The heart reads this value.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Stamina")
	bool IsRunning() const { return bRunning; }

	// True while the player holds the calm key.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Stamina")
	bool IsCalming() const { return bCalm; }

	// Keeps the aim offset inside the allowed box so the spine does not twist too far
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	static FRotator ClampAimOffset(const FRotator& Offset, float MaxYaw, float MaxPitch);

	bool IsAiming() const { return bAiming; }

	// True while the heart stress is high enough for the scared crouch idle
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Crouch")
	bool IsCrouchScared() const { return bCrouchScared; }

	// Fires when aiming starts or stops. Use it to show or hide a crosshair widget
	UFUNCTION(BlueprintImplementableEvent, Category = "CotS|Throw")
	void OnAimChanged(bool bNowAiming);
	
private:
	void ApplyMovementInput(const FInputActionValue& Value);
	void ApplyLookInput(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void UpdateCrouchCamera(float DeltaTime);
	void UpdateCrouchOrbit(float DeltaTime);
	void UpdateAimZoom(float DeltaTime);
	void PlayCrouchAnim(float Rate);
	void StartCrouchIdle();
	bool IsStandingUp() const;
	void StopCrouchIdle();
	void UpdateCrouchIdle();
	void PlayCrouchIdle(UAnimSequenceBase* Anim, float BlendSeconds);
	UAnimSequenceBase* GetWantedCrouchIdle() const;
	void ToggleCrouch();
	void StartJump();
	void Struggle();
	void UpdateStruggle(float DeltaTime);
	void FailStruggle();
	void DrainStruggle(float DeltaTime);
	void PlayPickedUpLoop();
	bool IsJumping() const;
	void ExitCrouch();
	void StartCalm();
	void StopCalm();
	void UpdateStamina(float DeltaTime);
	void StartTiredPause();
	bool IsTiredPausing() const;
	bool IsUsingGamepad() const;
	bool CanSprint() const { return !bExhausted && !bCalm && !bIsCrouched; }
	void SetSoundWorld(bool bSound);
	void DebugTogglePaint();
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

	// Root motion montage. It moves the character, so no physics jump is used
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Jump")
	TObjectPtr<UAnimMontage> JumpAnim{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Grab")
	TObjectPtr<UAnimMontage> PickedUpAnim{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Grab")
	TObjectPtr<UAnimMontage> HittingPickedUpAnim{};

	// 0 turns the time limit off, so only the QTE misses can end the game
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Grab")
	float StruggleTimeLimit{0.f};


	bool bPickedUp{};
	float StruggleProgress{};
	float StruggleTimeRemaining{};
	TWeakObjectPtr<AMonsterCharacter> PickedUpBy{};
	EVisibilityBasedAnimTickOption SavedAnimTickOption{};
	float PickedUpLogTimer{};
	void LogPickedUpTransforms(const TCHAR* Label) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Jump")
	float JumpUpImpulse{150.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Jump")
	float JumpForwardImpulse{150.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="1"), Category = "CotS|Jump")
	float JumpImpulseFraction{0.5f};

	FTimerHandle JumpImpulseTimer;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UAnimMontage> ThrowAnim{};

	// Max turn left or right, in degrees, while the player aims
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS")
	float CrouchingSpeed{450.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Crouch")
	// Stand up animation. The player plays it backward to crouch
	TObjectPtr<UAnimMontage> StandUpAnim{};

	// 1 is the montage speed. Lower is slower
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.05"), Category = "CotS|Crouch")
	float CrouchAnimRate{1.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Crouch")
	float CrouchDownSkipSeconds{0.1f};

	// Loops while the player stays crouched
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Crouch")
	TObjectPtr<UAnimSequenceBase> CrouchIdleAnim{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Crouch")
	FName CrouchSlotName{TEXT("DefaultSlot")};

	// Loops instead of CrouchIdleAnim while the heart stress is at CrouchScaredStress or higher
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Crouch")
	TObjectPtr<UAnimSequenceBase> CrouchScaredIdleAnim{};

	// 0.8 is 80% of the full heart stress
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="1"), Category = "CotS|Crouch")
	float CrouchScaredStress{0.8f};

	// The scared idle stops only below CrouchScaredStress minus this value, so it does not flicker
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="1"), Category = "CotS|Crouch")
	float CrouchScaredMargin{0.05f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Crouch")
	float CrouchIdleSwapBlendSeconds{0.3f};

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CrouchIdleMontage{};

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> CrouchIdlePlaying{};

	bool bCrouchIdleActive{};

	bool bCrouchScared{};

	// How fast the camera moves to the new height. Lower is slower
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.1"), Category = "CotS|Crouch")
	float CrouchCameraSpeed{3.f};

	float CameraHeightOffset{};

	// Lowest camera angle, in degrees. A negative value looks down
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="-89", ClampMax="0"), Category = "CotS|Camera")
	float MinCameraPitch{-50.f};

	// Highest camera angle, in degrees. A positive value looks up
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0", ClampMax="89"), Category = "CotS|Camera")
	float MaxCameraPitch{40.f};

	// Flips the up and down look for a gamepad only. The mouse stays the same
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS|Camera")
	bool bInvertGamepadPitch{true};

	bool bCameraReturning{};

	float CrouchStartTime{-100.f};

	double StandUpEndTime{};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Crouch")
	float CrouchMoveGraceSeconds{0.4f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0.1"), Category = "CotS|Crouch")
	float CrouchCameraReturnSpeed{5.f};

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS|Heartbeat")
	TObjectPtr<UCotSHeartbeatComponent> Heartbeat{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS|QTE")
	TObjectPtr<UCotSQTEPromptComponent> QTEPrompts{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"), Category = "CotS|QTE")
	TObjectPtr<UUIButtonDataAsset> QTEButtonData{};

	// Use WBP_NeutralToPressed after you set its parent class to CotSButtonPromptWidget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"), Category = "CotS|QTE")
	TSubclassOf<UCotSButtonPromptWidget> QTEPromptWidgetClass{};

	// Used when no menu has set a difficulty, for example when you play a level directly in the editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"), Category = "CotS|QTE")
	ECotSDifficulty QTEDifficulty{ECotSDifficulty::Normal};

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
	bool bRunning{};
	float Stamina{};
	
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="1"), Category = "CotS|Stamina")
	float MaxStamina{110.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float StaminaDrainPerSecond{25.f};

	// Time the player can keep sprinting at zero stamina before the exhausted pause starts
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess="true", ClampMin="0"), Category = "CotS|Stamina")
	float FullyTiredSeconds{5.f};

	float FullyTiredTime{};

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
