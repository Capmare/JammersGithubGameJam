// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "PaintEffectComponent.h"
#include "InteractableInterface.h"
#include "PlayerInputDataAsset.h"
#include "CotSMonsterDirector.h"
#include "Kismet/GameplayStatics.h"


APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;

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
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplySpeed(WalkingSpeed);
	Stamina = MaxStamina;

	const APlayerController* PlayerController{Cast<APlayerController>(GetController())};
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
	UpdateHeartbeat(DeltaTime);
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
	EnhancedInput->BindAction(InputReference->Paint, ETriggerEvent::Started, this, &APlayerCharacter::TogglePaint);
	EnhancedInput->BindAction(InputReference->Calm, ETriggerEvent::Started, this, &APlayerCharacter::StartCalm);
	EnhancedInput->BindAction(InputReference->Calm, ETriggerEvent::Completed, this, &APlayerCharacter::StopCalm);
	EnhancedInput->BindAction(InputReference->Interact, ETriggerEvent::Started, this, &APlayerCharacter::TryPickUp);
}

void APlayerCharacter::ApplyMovementInput(const FInputActionValue& Value)
{
	if (bCalm) return;

	const FVector2D Clamped{Value.Get<FVector2D>().GetClampedToMaxSize(1.0)};
	AddMovementInput(GetActorForwardVector(), Clamped.Y);
	AddMovementInput(GetActorRightVector(), Clamped.X);
}

void APlayerCharacter::ApplyLookInput(const FInputActionValue& Value)
{
	const FVector2D Look{Value.Get<FVector2D>()};
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(-Look.Y);
}

void APlayerCharacter::StartSprint()
{
	bSprintHeld = true;
	if (CanSprint()) ApplySpeed(SprintingSpeed);
}

void APlayerCharacter::StopSprint()
{
	bSprintHeld = false;
	ApplySpeed(WalkingSpeed);
}

void APlayerCharacter::StartCalm()
{
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
	const bool bSprinting{bSprintHeld && CanSprint() && !GetVelocity().IsNearlyZero()};

	if (bSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainPerSecond * DeltaTime);
		if (Stamina <= 0.f)
		{
			bExhausted = true;
			ApplySpeed(WalkingSpeed);
		}
		return;
	}

	Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenPerSecond * (bCalm ? CalmRegenMultiplier : 1.f) * DeltaTime);
	if (bExhausted && Stamina >= SprintResumeStamina)
	{
		bExhausted = false;
		if (bSprintHeld && !bCalm) ApplySpeed(SprintingSpeed);
	}
}

void APlayerCharacter::UpdateHeartbeat(const float DeltaTime)
{
	if (!HeartbeatSound) return;

	HeartbeatTimer -= DeltaTime;
	if (HeartbeatTimer > 0.f) return;

	const float StaminaStress{1.f - Stamina / MaxStamina};

	float MonsterFear{};
	if (const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)})
	{
		MonsterFear = 1.f - FMath::Clamp(Director->GetNearestMonsterDistance(GetActorLocation()) / MonsterFearDistance, 0.f, 1.f);
	}

	float Intensity{FMath::Max(StaminaStress, MonsterFear)};
	if (bCalm) Intensity *= CalmIntensityScale;

	UGameplayStatics::PlaySound2D(this, HeartbeatSound);
	HeartbeatTimer = FMath::Lerp(SlowestBeatInterval, FastestBeatInterval, Intensity);
}

void APlayerCharacter::TogglePaint()
{
	bPaintActive = !bPaintActive;

	if (bPaintActive) PaintEffect->Start();
	else PaintEffect->Reverse();
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

	FHitResult Hit{};

	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

#if WITH_EDITOR
	if (ShowPickUpDebugRay)
	{
		DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 5.0f, 0, 5.0f);
	}
#endif
	
	if (!bHit)
	{
		return nullptr;
	}

	AActor* HitActor{Hit.GetActor()};

	if (!IsValid(HitActor))
	{
		return nullptr;
	}

	if (!HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
	{
		return nullptr;
	}

	return HitActor;
}

void APlayerCharacter::TryPickUp()
{
	// UE_LOG(LogTemp, Log, TEXT("Trying Pickup!"));
	
	AActor* TargetActor{FindPickupCandidate()};

	if (!TargetActor)
	{
		// UE_LOG(LogTemp, Log, TEXT("No Pickupable found!"));
		return;
	}

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

	//UE_LOG(LogTemp, Log, TEXT("Player Picked Up %s"), *TargetActor->GetName());
}
