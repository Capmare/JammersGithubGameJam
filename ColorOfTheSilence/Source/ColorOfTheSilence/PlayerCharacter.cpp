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
#include "PlayerInputDataAsset.h"


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
}

void APlayerCharacter::ApplyMovementInput(const FInputActionValue& Value)
{
	const FVector2D Clamped{Value.Get<FVector2D>().GetClampedToMaxSize(1.0)};
	AddMovementInput(GetActorForwardVector(), Clamped.Y);
	AddMovementInput(GetActorRightVector(), Clamped.X);
}

void APlayerCharacter::ApplyLookInput(const FInputActionValue& Value)
{
	const FVector2D Look{Value.Get<FVector2D>()};
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y);
}

void APlayerCharacter::StartSprint()
{
	ApplySpeed(SprintingSpeed);
}

void APlayerCharacter::StopSprint()
{
	ApplySpeed(WalkingSpeed);
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
