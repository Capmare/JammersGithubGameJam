// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "PaintEffectComponent.h"
#include "CotSHeartbeatComponent.h"
#include "InteractableInterface.h"
#include "PlayerInputDataAsset.h"
#include "ThrowablePickup.h"
#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "MonsterCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"


APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchingSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 250.f;
	CameraBoom->SocketOffset = FVector{0.f, 51.076555f, 26.390126f};
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->ProbeSize = 4.f;
	CameraBoom->bDoCollisionTest = true;

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	PlayerCamera->bUsePawnControlRotation = false;

	PaintEffect = CreateDefaultSubobject<UPaintEffectComponent>(TEXT("PaintEffect"));
	Heartbeat = CreateDefaultSubobject<UCotSHeartbeatComponent>(TEXT("Heartbeat"));
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchingSpeed;
	DefaultFieldOfView = PlayerCamera->FieldOfView;
	DefaultArmLength = CameraBoom->TargetArmLength;
	ApplySpeed(WalkingSpeed);
	Stamina = MaxStamina;

	const APlayerController* PlayerController{Cast<APlayerController>(GetController())};
	if (PlayerController && PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->ViewPitchMin = MinCameraPitch;
		PlayerController->PlayerCameraManager->ViewPitchMax = MaxCameraPitch;
	}
	if (!PlayerController || !InputMappingReference) return;

	if (auto* Subsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())})
	{
		Subsystem->AddMappingContext(InputMappingReference, 0);
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateStamina(DeltaTime);
	// Only X and Y change, so gravity still works
	FVector& Velocity{GetCharacterMovement()->Velocity};
	if (bAiming)
	{
		Velocity.X = 0.0;
		Velocity.Y = 0.0;
	}
	else if (IsTiredPausing())
	{
		const FVector Slowed{FMath::VInterpTo(Velocity, FVector{0.0, 0.0, Velocity.Z}, DeltaTime, ExhaustedSlowDownSpeed)};
		Velocity.X = Slowed.X;
		Velocity.Y = Slowed.Y;
	}
	if (bAiming) UpdateAimPreview();
	UpdateCrouchCamera(DeltaTime);
	if (bPickedUp) UpdateStruggle(DeltaTime);
	UpdateCrouchOrbit(DeltaTime);
	UpdateAimZoom(DeltaTime);
}

void APlayerCharacter::OnStartCrouch(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CameraHeightOffset += ScaledHalfHeightAdjust;
}

void APlayerCharacter::OnEndCrouch(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	CameraHeightOffset -= ScaledHalfHeightAdjust;
}

// The capsule moves at once. This offset hides the jump and eases the camera to its new height
void APlayerCharacter::UpdateCrouchCamera(const float DeltaTime)
{
	CameraHeightOffset = FMath::FInterpTo(CameraHeightOffset, 0.f, DeltaTime, CrouchCameraSpeed);
	CameraBoom->TargetOffset = FVector{0.f, 0.f, CameraHeightOffset};
}

void APlayerCharacter::UpdateCrouchOrbit(const float DeltaTime)
{
	if (bIsCrouched) bCameraReturning = true;
	else if (bCameraReturning)
	{
		if (AController* PlayerController{GetController()})
		{
			FRotator Control{PlayerController->GetControlRotation()};
			Control.Yaw = FMath::FInterpTo(Control.Yaw, Control.Yaw + FRotator::NormalizeAxis(GetActorRotation().Yaw - Control.Yaw), DeltaTime, CrouchCameraReturnSpeed);
			PlayerController->SetControlRotation(Control);
			if (FMath::Abs(FRotator::NormalizeAxis(GetActorRotation().Yaw - Control.Yaw)) < 1.f) bCameraReturning = false;
		}
		else bCameraReturning = false;
	}

	bUseControllerRotationYaw = !bPickedUp && (bAiming || !bCameraReturning);
}

void APlayerCharacter::UpdateAimZoom(const float DeltaTime)
{
	const float TargetFieldOfView{bAiming ? AimFieldOfView : DefaultFieldOfView};
	const float TargetArmLength{bAiming ? AimArmLength : DefaultArmLength};

	PlayerCamera->SetFieldOfView(FMath::FInterpTo(PlayerCamera->FieldOfView, TargetFieldOfView, DeltaTime, AimZoomSpeed));
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, AimZoomSpeed);
}

void APlayerCharacter::PlayCrouchAnim(const float Rate)
{
	StopCrouchIdle();

	UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	if (!StandUpAnim || !AnimInstance)
	{
		if (Rate < 0.f) StartCrouchIdle();
		return;
	}

	const float StartTime{Rate < 0.f ? FMath::Max(0.f, static_cast<float>(StandUpAnim->GetPlayLength()) - CrouchDownSkipSeconds) : 0.f};
	if (Rate < 0.f)
	{
		AnimInstance->Montage_Play(StandUpAnim, Rate, EMontagePlayReturnType::MontageLength, StartTime);
	}
	else
	{
		StandUpEndTime = GetWorld()->GetTimeSeconds() + StandUpAnim->GetPlayLength() / Rate;
		AnimInstance->Montage_PlayWithBlendSettings(StandUpAnim, FMontageBlendSettings{0.f}, Rate, EMontagePlayReturnType::MontageLength, StartTime);
	}

	if (Rate < 0.f)
	{
		FOnMontageBlendingOutStarted BlendOut{};
		BlendOut.BindWeakLambda(this, [this](UAnimMontage*, const bool bInterrupted)
		{
			if (!bInterrupted && bIsCrouched) StartCrouchIdle();
		});
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOut, StandUpAnim);
	}
}

bool APlayerCharacter::IsStandingUp() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < StandUpEndTime;
}

void APlayerCharacter::StartCrouchIdle()
{
	UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	if (!CrouchIdleAnim || !AnimInstance || !bIsCrouched) return;

	CrouchIdleMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(CrouchIdleAnim, CrouchSlotName, 0.2f, 0.2f, 1.f, MAX_int32);
}

void APlayerCharacter::StopCrouchIdle()
{
	UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	if (CrouchIdleMontage && AnimInstance) AnimInstance->Montage_Stop(0.f, CrouchIdleMontage);
	CrouchIdleMontage = nullptr;
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto* EnhancedInput{Cast<UEnhancedInputComponent>(PlayerInputComponent)};
	if (!EnhancedInput || !InputReference || !InputReference->CheckAllBindingsValid()) return;

	EnhancedInput->BindAction(InputReference->Movement, ETriggerEvent::Triggered, this, &APlayerCharacter::ApplyMovementInput);
	EnhancedInput->BindAction(InputReference->Look, ETriggerEvent::Triggered, this, &APlayerCharacter::ApplyLookInput);
	EnhancedInput->BindAction(InputReference->Sprint, ETriggerEvent::Started, this, &APlayerCharacter::StartSprint);
	EnhancedInput->BindAction(InputReference->Sprint, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);
#if !UE_BUILD_SHIPPING
	EnhancedInput->BindAction(InputReference->Paint, ETriggerEvent::Started, this, &APlayerCharacter::DebugTogglePaint);
#endif
	EnhancedInput->BindAction(InputReference->Calm, ETriggerEvent::Started, this, &APlayerCharacter::StartCalm);
	EnhancedInput->BindAction(InputReference->Calm, ETriggerEvent::Completed, this, &APlayerCharacter::StopCalm);
	EnhancedInput->BindAction(InputReference->Interact, ETriggerEvent::Started, this, &APlayerCharacter::TryPickUp);
	EnhancedInput->BindAction(InputReference->Crouch, ETriggerEvent::Started, this, &APlayerCharacter::ToggleCrouch);
	EnhancedInput->BindAction(InputReference->Aim, ETriggerEvent::Started, this, &APlayerCharacter::StartAim);
	EnhancedInput->BindAction(InputReference->Aim, ETriggerEvent::Completed, this, &APlayerCharacter::CancelAim);
	EnhancedInput->BindAction(InputReference->Aim, ETriggerEvent::Canceled, this, &APlayerCharacter::CancelAim);
	EnhancedInput->BindAction(InputReference->Throw, ETriggerEvent::Started, this, &APlayerCharacter::ThrowHeldItem);
	if (InputReference->Struggle) EnhancedInput->BindAction(InputReference->Struggle, ETriggerEvent::Started, this, &APlayerCharacter::Struggle);
	if (InputReference->Jump) EnhancedInput->BindAction(InputReference->Jump, ETriggerEvent::Started, this, &APlayerCharacter::StartJump);
}

void APlayerCharacter::ApplyMovementInput(const FInputActionValue& Value)
{
	if (bPickedUp) return;
	if (bCalm || bAiming || IsTiredPausing() || IsJumping() || IsStandingUp()) return;

	const FVector2D Clamped{Value.Get<FVector2D>().GetClampedToMaxSize(1.0)};
	if (!Clamped.IsNearlyZero() && GetWorld()->GetTimeSeconds() - CrouchStartTime > CrouchMoveGraceSeconds) ExitCrouch();
	if (IsStandingUp()) return;
	AddMovementInput(GetActorForwardVector(), Clamped.Y);
	AddMovementInput(GetActorRightVector(), Clamped.X);
}

void APlayerCharacter::ApplyLookInput(const FInputActionValue& Value)
{
	if (bPickedUp) return;
	if (IsTiredPausing()) return;

	const FVector2D Look{Value.Get<FVector2D>()};

	if (bAiming)
	{
		AimOffset = ClampAimOffset(AimOffset + FRotator{Look.Y, Look.X, 0.f}, MaxAimYaw, MaxAimPitch);
		if (AController* PlayerController{GetController()})
		{
			PlayerController->SetControlRotation((AimBaseRotation + AimOffset).GetNormalized());
		}
		return;
	}

	AddControllerYawInput(Look.X);
	AddControllerPitchInput(-Look.Y);
}

FRotator APlayerCharacter::ClampAimOffset(const FRotator& Offset, const float MaxYaw, const float MaxPitch)
{
	return FRotator{
		FMath::Clamp(FRotator::NormalizeAxis(Offset.Pitch), -MaxPitch, MaxPitch),
		FMath::Clamp(FRotator::NormalizeAxis(Offset.Yaw), -MaxYaw, MaxYaw),
		0.f};
}

void APlayerCharacter::StartAim()
{
	if (bPickedUp) return;
	if (bAiming || !IsValid(HeldItem) || bCalm) return;

	const AController* PlayerController{GetController()};
	if (!PlayerController) return;

	bAiming = true;
	AimBaseRotation = PlayerController->GetControlRotation();
	AimOffset = FRotator::ZeroRotator;
	ApplySpeed(WalkingSpeed);
	GetCharacterMovement()->StopMovementImmediately();
	OnAimChanged(true);
}

void APlayerCharacter::CancelAim()
{
	const bool bWasAiming{bAiming};
	bAiming = false;
	if (bWasAiming) OnAimChanged(false);
	AimOffset = FRotator::ZeroRotator;
	if (bSprintHeld && CanSprint()) ApplySpeed(SprintingSpeed);
}

void APlayerCharacter::ThrowHeldItem()
{
	if (bPickedUp) return;
	if (!bAiming) return;

	AActor* Held{HeldItem};
	const AController* PlayerController{GetController()};
	CancelAim();

	if (!IsValid(Held) || !PlayerController) return;

	if (ThrowAnim) PlayAnimMontage(ThrowAnim);

	const FVector Direction{GetThrowDirection()};

	FActorSpawnParameters SpawnParams{};
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = this;
	SpawnParams.bDeferConstruction = true;

	AActor* Thrown{GetWorld()->SpawnActor<AActor>(Held->GetClass(), FTransform{Direction.Rotation(), GetThrowStart()}, SpawnParams)};

	Held->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Held->Destroy();
	HeldItem = nullptr;

	if (!Thrown) return;

	AThrowablePickup* HeldThrowable{Cast<AThrowablePickup>(Held)};
	if (AThrowablePickup* ThrownThrowable{Cast<AThrowablePickup>(Thrown)})
	{
		if (HeldThrowable) ThrownThrowable->UseMesh(HeldThrowable->GetCurrentMesh());
		ThrownThrowable->MarkThrown();
	}

	Thrown->FinishSpawning(FTransform{Direction.Rotation(), GetThrowStart()});

	if (AThrowablePickup* ThrownThrowable{Cast<AThrowablePickup>(Thrown)})
	{
		ThrownThrowable->Launch(Direction * ThrowSpeed);
	}
	else if (UPrimitiveComponent* Body{Cast<UPrimitiveComponent>(Thrown->GetRootComponent())})
	{
		Body->SetSimulatePhysics(true);
		Body->SetPhysicsLinearVelocity(Direction * ThrowSpeed);
	}
}

FVector APlayerCharacter::GetThrowDirection() const
{
	return GetController() ? GetController()->GetControlRotation().Vector() : GetActorForwardVector();
}

FVector APlayerCharacter::GetThrowStart() const
{
	return GetPawnViewLocation() + GetThrowDirection() * ThrowSpawnDistance;
}

void APlayerCharacter::UpdateAimPreview() const
{
	if (!bShowThrowPreview || !IsValid(HeldItem)) return;

	FPredictProjectilePathParams Params{};
	Params.StartLocation = GetThrowStart();
	Params.LaunchVelocity = GetThrowDirection() * ThrowSpeed;
	Params.ProjectileRadius = 5.f;
	Params.MaxSimTime = PreviewSeconds;
	Params.SimFrequency = 15.f;
	Params.bTraceWithCollision = true;
	Params.TraceChannel = ECC_Visibility;
	Params.ActorsToIgnore = {const_cast<APlayerCharacter*>(this), HeldItem};

	FPredictProjectilePathResult Result{};
	UGameplayStatics::PredictProjectilePath(this, Params, Result);

	for (int32 Index{1}; Index < Result.PathData.Num(); ++Index)
	{
		DrawDebugLine(GetWorld(), Result.PathData[Index - 1].Location, Result.PathData[Index].Location, FColor::White, false, -1.f, 0, 2.f);
	}

	DrawDebugSphere(GetWorld(), Result.LastTraceDestination.Location, 12.f, 8, Result.HitResult.bBlockingHit ? FColor::Green : FColor::Red, false, -1.f, 0, 1.5f);
}

void APlayerCharacter::StartSprint()
{
	if (bPickedUp) return;
	bSprintHeld = true;
	if (CanSprint()) ApplySpeed(SprintingSpeed);
}

void APlayerCharacter::StopSprint()
{
	bSprintHeld = false;
	ApplySpeed(WalkingSpeed);
}

void APlayerCharacter::ToggleCrouch()
{
	if (bPickedUp) return;
	if (IsStandingUp()) return;
	if (bAiming) return;

	if (bIsCrouched)
	{
		ExitCrouch();
		return;
	}

	if (!CanCrouch()) return;

	Crouch();
	CrouchStartTime = GetWorld()->GetTimeSeconds();
	ApplySpeed(WalkingSpeed);
	PlayCrouchAnim(-CrouchAnimRate);
}

bool APlayerCharacter::IsJumping() const
{
	const UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	return JumpAnim && AnimInstance && AnimInstance->Montage_IsPlaying(JumpAnim);
}

void APlayerCharacter::StartJump()
{
	if (bPickedUp) return;
	if (!JumpAnim || bAiming || bCalm || IsTiredPausing() || IsJumping()) return;
	if (GetCharacterMovement()->IsFalling()) return;

	if (bIsCrouched) ExitCrouch();
	if (UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()})
	{
		AnimInstance->Montage_Play(JumpAnim);
		const float Delay{static_cast<float>(JumpAnim->GetPlayLength()) * JumpImpulseFraction};
		GetWorldTimerManager().SetTimer(JumpImpulseTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (!IsJumping()) return;
			LaunchCharacter(GetActorForwardVector() * JumpForwardImpulse + FVector{0.0, 0.0, static_cast<double>(JumpUpImpulse)}, false, false);
		}), FMath::Max(Delay, KINDA_SMALL_NUMBER), false);
	}
}

void APlayerCharacter::ExitCrouch()
{
	if (!bIsCrouched || !GetCharacterMovement()->bWantsToCrouch) return;

	UnCrouch();
	PlayCrouchAnim(CrouchAnimRate);
	if (bSprintHeld && CanSprint()) ApplySpeed(SprintingSpeed);
}

void APlayerCharacter::StartCalm()
{
	if (bPickedUp) return;
	bCalm = true;
	ApplySpeed(WalkingSpeed);
	GetCharacterMovement()->StopMovementImmediately();
}

void APlayerCharacter::StopCalm()
{
	bCalm = false;
	if (bSprintHeld && CanSprint()) ApplySpeed(SprintingSpeed);
}

void APlayerCharacter::UpdateStamina(const float DeltaTime)
{
	const bool bRunningOutOfBreath{Stamina <= 0.f && FullyTiredTime > 0.f};
	const bool bSprinting{bSprintHeld && CanSprint() && (bRunningOutOfBreath || !GetVelocity().IsNearlyZero())};
	// The heart reads this value through IsRunning
	bRunning = bSprinting;

	if (bSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainPerSecond * DeltaTime);
		if (Stamina <= 0.f)
		{
			FullyTiredTime += DeltaTime;
			const float Alpha{FMath::Clamp(FullyTiredTime / FMath::Max(FullyTiredSeconds, KINDA_SMALL_NUMBER), 0.f, 1.f)};
			ApplySpeed(FMath::Lerp(SprintingSpeed, 0.f, Alpha));
		}
		if (FullyTiredTime >= FullyTiredSeconds)
		{
			FullyTiredTime = 0.f;
			bExhausted = true;
			ApplySpeed(WalkingSpeed);
			StartTiredPause();
		}
		return;
	}

	FullyTiredTime = 0.f;
	Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * (bCalm ? CalmRegenMultiplier : 1.f) * DeltaTime);
	if (bExhausted && Stamina >= SprintResumeStamina)
	{
		bExhausted = false;
		if (bSprintHeld && !bCalm) ApplySpeed(SprintingSpeed);
	}
}

bool APlayerCharacter::IsTiredPausing() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < TiredPauseEndTime;
}

void APlayerCharacter::StartTiredPause()
{
	float MontageLength{};
	if (ExhaustedAnim)
	{
		if (UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()})
		{
			FMontageBlendSettings BlendIn{ExhaustedBlendInSeconds};
			BlendIn.Blend.BlendOption = EAlphaBlendOption::HermiteCubic;
			MontageLength = AnimInstance->Montage_PlayWithBlendSettings(ExhaustedAnim, BlendIn);
		}
	}
	const float Pause{MontageLength > 0.f ? MontageLength : static_cast<float>(FMath::FRandRange(MinExhaustedPause, FMath::Max(MinExhaustedPause, MaxExhaustedPause)))};
	TiredPauseEndTime = GetWorld()->GetTimeSeconds() + Pause;
}

void APlayerCharacter::SetSoundWorld(const bool bSound)
{
	if (bPaintActive == bSound) return;

	bPaintActive = bSound;
	if (!IsValid(PaintEffect)) return;

	if (!bPaintActive)
	{
		PaintEffect->Reverse();
		return;
	}

	// The heart opens a ripple on each beat, so the sound world starts with the ripple of one beat.
	if (IsValid(Heartbeat) && Heartbeat->bPulseReveal) PaintEffect->PulseRadius(Heartbeat->GetRevealRadius(), Heartbeat->GetBeatInterval());
	else PaintEffect->Start();
}

void APlayerCharacter::DebugTogglePaint()
{
	SetSoundWorld(!bPaintActive);
}

void APlayerCharacter::BeginWorldSwapping()
{
	if (bWorldSwapping) return;

	bWorldSwapping = true;
	SwapRandom.Initialize(static_cast<int32>(FPlatformTime::Cycles64()));
	SetSoundWorld(true);
	ScheduleNextWorldSwap();
}

void APlayerCharacter::ScheduleNextWorldSwap()
{
	const float Wait{static_cast<float>(SwapRandom.FRandRange(MinWorldSwapSeconds, FMath::Max(MinWorldSwapSeconds, MaxWorldSwapSeconds)))};
	GetWorldTimerManager().SetTimer(WorldSwapTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetSoundWorld(!bPaintActive);
		ScheduleNextWorldSwap();
	}), Wait, false);
}

void APlayerCharacter::ApplySpeed(const float NewSpeed) const
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

AActor* APlayerCharacter::FindPickupCandidate()
{
	UWorld* World{ GetWorld() };
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PlayerController{Cast<APlayerController>(GetController())};

	if (!PlayerController)
	{
		return nullptr;
	}

	FVector ViewLocation{};
	FRotator ViewRotation{};

	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector Start{ViewLocation};
	const FVector End{Start + ViewRotation.Vector() * PickUpDistance};

	FCollisionQueryParams QueryParams{};
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> Hits{};
	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(PickUpRadius), QueryParams);

	// The sweep ends at the first blocking hit, so a wall hides what is behind it
	AActor* Candidate{};
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor{Hit.GetActor()};
		if (IsValid(HitActor) && HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		{
			Candidate = HitActor;
			break;
		}
	}

#if WITH_EDITOR
	if (ShowPickUpDebugRay)
	{
		const FColor Color{Candidate ? FColor::Green : FColor::Red};
		DrawDebugLine(World, Start, End, Color, false, 5.0f, 0, 2.0f);
		DrawDebugSphere(World, End, PickUpRadius, 12, Color, false, 5.0f, 0, 1.0f);
	}
#endif

	return Candidate;
}

void APlayerCharacter::TryPickUp()
{
	if (bPickedUp) return;
	// UE_LOG(LogTemp, Log, TEXT("Trying Pickup!"));
	
	AActor* TargetActor{FindPickupCandidate()};

	if (!TargetActor)
	{
		// UE_LOG(LogTemp, Log, TEXT("No Pickupable found!"));
		return;
	}

	// The pickup can destroy itself in Execute_PickUp, so the animation plays first
	if (PickupAnim) PlayAnimMontage(PickupAnim);

	IInteractableInterface::Execute_PickUp(TargetActor, this);
	PickUp(TargetActor);
}

// Currently unused, actions can be done here on the player itself
void APlayerCharacter::PickUp(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	HeldItem = TargetActor;
	TargetActor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocketName);

	//UE_LOG(LogTemp, Log, TEXT("Player Picked Up %s"), *TargetActor->GetName());
}

void APlayerCharacter::BeginPickedUp(AMonsterCharacter* Monster)
{
	if (bPickedUp || !IsValid(Monster)) return;

	bPickedUp = true;
	PickedUpBy = Monster;
	StruggleProgress = 0.f;
	StruggleTimeRemaining = StruggleTimeLimit;

	CancelAim();
	if (bCalm) StopCalm();
	StopCrouchIdle();
	UnCrouch();
	bSprintHeld = false;

	UCharacterMovementComponent* Movement{GetCharacterMovement()};
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	SetActorEnableCollision(false);
	LogPickedUpTransforms(TEXT("before pickup"));

	SetActorLocation(Monster->GetHoldLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator{0.0, Monster->GetActorRotation().Yaw + 180.0, 0.0});

	SavedAnimTickOption = GetMesh()->VisibilityBasedAnimTickOption;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	PickedUpLogTimer = 0.f;
	LogPickedUpTransforms(TEXT("after pickup"));

	if (APlayerController* PlayerController{Cast<APlayerController>(GetController())})
	{
		if (UCameraComponent* GrabCamera{Monster->GetGrabCamera()})
		{
			GrabCamera->Activate();
			PlayerController->SetViewTargetWithBlend(Monster, Monster->GetGrabCameraBlendTime());
		}
	}

	PlayPickedUpLoop();

	GetWorldTimerManager().PauseTimer(WorldSwapTimer);
	SetSoundWorld(true);

	OnPickedUp.Broadcast(Monster);
	OnStruggleStarted.Broadcast(StruggleTimeLimit);
	OnStruggleProgressChanged.Broadcast(0.f);
}

void APlayerCharacter::EndPickedUp(const bool bEscaped)
{
	if (!bPickedUp) return;

	bPickedUp = false;
	AMonsterCharacter* Monster{PickedUpBy.Get()};
	PickedUpBy.Reset();

	SetSoundWorld(false);
	GetWorldTimerManager().UnPauseTimer(WorldSwapTimer);

	LogPickedUpTransforms(TEXT("before release"));
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorEnableCollision(true);

	GetMesh()->VisibilityBasedAnimTickOption = SavedAnimTickOption;

	if (UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()})
	{
		if (PickedUpAnim) AnimInstance->Montage_Stop(0.2f, PickedUpAnim);
		if (HittingPickedUpAnim) AnimInstance->Montage_Stop(0.2f, HittingPickedUpAnim);
	}

	if (!bEscaped) return;

	UCharacterMovementComponent* Movement{GetCharacterMovement()};
	Movement->SetDefaultMovementMode();

	APlayerController* PlayerController{Cast<APlayerController>(GetController())};
	if (Monster)
	{
		const FVector MonsterLocation{Monster->GetActorLocation()};
		const FVector Forward{Monster->GetActorForwardVector()};
		SetActorLocation(MonsterLocation + Forward * Monster->GetEscapeDistance(), false, nullptr, ETeleportType::TeleportPhysics);

		const FRotator Facing{0.0, (-Forward).Rotation().Yaw, 0.0};
		SetActorRotation(Facing);
		if (PlayerController) PlayerController->SetControlRotation(Facing);

		if (UCameraComponent* GrabCamera{Monster->GetGrabCamera()}) GrabCamera->Deactivate();
		if (PlayerController) PlayerController->SetViewTargetWithBlend(this, Monster->GetGrabCameraBlendTime());
		Monster->ReleasePlayer();
	}
	else if (PlayerController)
	{
		PlayerController->SetViewTargetWithBlend(this, 0.f);
	}

	OnEscaped.Broadcast();
}

void APlayerCharacter::Struggle()
{
	if (!bPickedUp) return;

	StruggleProgress = FMath::Clamp(StruggleProgress + StruggleProgressPerPress, 0.f, 1.f);
	OnStrugglePressed.Broadcast();
	OnStruggleProgressChanged.Broadcast(StruggleProgress);

	if (StruggleProgress >= 1.f)
	{
		EndPickedUp(true);
		return;
	}

	UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	if (!AnimInstance || !HittingPickedUpAnim) return;

	AnimInstance->Montage_Play(HittingPickedUpAnim);
	FOnMontageEnded EndDelegate{};
	EndDelegate.BindWeakLambda(this, [this](UAnimMontage*, const bool bInterrupted)
	{
		if (!bInterrupted && bPickedUp) PlayPickedUpLoop();
	});
	AnimInstance->Montage_SetEndDelegate(EndDelegate, HittingPickedUpAnim);
}

void APlayerCharacter::UpdateStruggle(const float DeltaTime)
{
	if (const AMonsterCharacter* Holder{PickedUpBy.Get()})
	{
		SetActorLocation(Holder->GetHoldLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	PickedUpLogTimer += DeltaTime;
	if (PickedUpLogTimer >= 1.f)
	{
		PickedUpLogTimer = 0.f;
		LogPickedUpTransforms(TEXT("holding"));
	}

	if (StruggleTimeRemaining <= 0.f) return;

	StruggleTimeRemaining -= DeltaTime;
	if (StruggleTimeRemaining <= 0.f)
	{
		StruggleTimeRemaining = 0.f;
		OnCaptureFailed.Broadcast();
		if (UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(this)}) Objectives->ReportPlayerCaught();
		return;
	}

	const float Decayed{FMath::Max(0.f, StruggleProgress - StruggleDecayPerSecond * DeltaTime)};
	if (!FMath::IsNearlyEqual(Decayed, StruggleProgress))
	{
		StruggleProgress = Decayed;
		OnStruggleProgressChanged.Broadcast(StruggleProgress);
	}
}

void APlayerCharacter::PlayPickedUpLoop()
{
	UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	if (!bPickedUp || !PickedUpAnim || !AnimInstance) return;

	AnimInstance->Montage_Play(PickedUpAnim);
	if (PickedUpAnim->GetNumSections() > 0)
	{
		const FName Section{PickedUpAnim->GetSectionName(0)};
		AnimInstance->Montage_SetNextSection(Section, Section, PickedUpAnim);
	}
	FOnMontageEnded EndDelegate{};
	EndDelegate.BindWeakLambda(this, [this](UAnimMontage*, const bool bInterrupted)
	{
		if (!bInterrupted) PlayPickedUpLoop();
	});
	AnimInstance->Montage_SetEndDelegate(EndDelegate, PickedUpAnim);
}

void APlayerCharacter::LogPickedUpTransforms(const TCHAR* Label) const
{
	const AMonsterCharacter* Monster{PickedUpBy.Get()};
	const USkeletalMeshComponent* MonsterMesh{Monster ? Monster->GetMesh() : nullptr};
	const FTransform Socket{MonsterMesh ? MonsterMesh->GetSocketTransform(Monster->GetHoldSocketName()) : FTransform::Identity};
	const FTransform Actor{GetActorTransform()};
	const FTransform MeshTransform{GetMesh()->GetComponentTransform()};

	UE_LOG(LogCotS, Log, TEXT("Pickup %s | monster actor %s | player actor loc %s rot %s scale %s | player mesh scale %s rot %s | socket loc %s rot %s scale %s | monster mesh scale %s"),
		Label,
		Monster ? *Monster->GetActorLocation().ToCompactString() : TEXT("none"),
		*Actor.GetLocation().ToCompactString(), *Actor.Rotator().ToCompactString(), *Actor.GetScale3D().ToCompactString(),
		*MeshTransform.GetScale3D().ToCompactString(), *MeshTransform.Rotator().ToCompactString(),
		*Socket.GetLocation().ToCompactString(), *Socket.Rotator().ToCompactString(), *Socket.GetScale3D().ToCompactString(),
		MonsterMesh ? *MonsterMesh->GetComponentScale().ToCompactString() : TEXT("none"));
}
