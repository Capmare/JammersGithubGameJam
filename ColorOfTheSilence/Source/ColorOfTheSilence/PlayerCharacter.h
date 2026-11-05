// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;
class UPlayerInputDataAsset;
class UCameraComponent;
class USpringArmComponent;
class UPaintEffectComponent;
class UInputMappingContext;

enum class EPlayerWalkingStatus
{
	Idle,
	Walking,
	Sprinting
};

UCLASS()
class COLOROFTHESILENCE_API APlayerCharacter : public ACharacter
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
	
private:
	void ApplyMovementInput(const FInputActionValue& Value);
	void ApplyLookInput(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void TogglePaint();
	void ApplySpeed(float NewSpeed) const;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UPlayerInputDataAsset> InputReference{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<USpringArmComponent> CameraBoom{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UPaintEffectComponent> PaintEffect{};

	bool bPaintActive{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UCameraComponent> PlayerCamera{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	TObjectPtr<UInputMappingContext> InputMappingReference{};
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	float WalkingSpeed{600.f};

	UPROPERTY(BlueprintReadWrite,EditAnywhere, meta=(AllowPrivateAccess="true"), Category = "CotS")
	float SprintingSpeed{900.f};
};
