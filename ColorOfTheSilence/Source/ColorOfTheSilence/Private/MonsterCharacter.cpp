#include "MonsterCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "MonsterController.h"
#include "MonsterFreezeComponent.h"
#include "../PlayerCharacter.h"

AMonsterCharacter::AMonsterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	FreezeComponent = CreateDefaultSubobject<UMonsterFreezeComponent>(TEXT("FreezeComponent"));

	GrabCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GrabCamera"));
	GrabCamera->SetupAttachment(RootComponent);
	GrabCamera->SetAutoActivate(false);
	GrabCamera->SetRelativeLocation(FVector{250.0, 0.0, 120.0});
	GrabCamera->SetRelativeRotation(FRotator{0.0, 180.0, 0.0});

	AIControllerClass = AMonsterController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->RegisterMonster(this);

#if !UE_BUILD_SHIPPING
	FTimerHandle StatusTimer{};
	GetWorldTimerManager().SetTimer(StatusTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		const AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};
		const APawn* Player{UGameplayStatics::GetPlayerPawn(this, 0)};
		const UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
		UE_LOG(LogCotS, Log, TEXT("Monster %s: mode %s | at %s | frozen %d | dormant %d grabbing %d attacking %d fleeing %d stalking %d | speed %.0f max %.0f | player dist %.0f | anim %s montage %s | AI %s"),
			*GetName(), GetModeLabel(), *GetActorLocation().ToCompactString(), FreezeComponent && FreezeComponent->IsFrozen(), bDormant, bGrabbing, bAttacking, bFleeing, bStalking, GetVelocity().Size(), GetCharacterMovement()->MaxWalkSpeed,
			Player ? FVector::Dist(GetActorLocation(), Player->GetActorLocation()) : -1.0,
			*GetNameSafe(AnimInstance), AnimInstance ? *GetNameSafe(AnimInstance->GetCurrentActiveMontage()) : TEXT("none"),
			MonsterController ? *MonsterController->DescribeBehavior() : TEXT("no controller"));
	}), 1.f, true);
#endif
}

void AMonsterCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(GrabTimer);
	GetWorldTimerManager().ClearTimer(StunTimer);
	GetWorldTimerManager().ClearTimer(StalkExitTimer);
	if (bFleeing) GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;
	if (APlayerCharacter* Player{GrabbedPlayer.Get()}) Player->EndPickedUp(true);
	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->UnregisterMonster(this);

	Super::EndPlay(Reason);
}

void AMonsterCharacter::SetDormant(const bool bNewDormant)
{
	if (bNewDormant && ShouldAlwaysSpawn()) return;

	AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};

	if (bNewDormant != bDormant)
	{
		bDormant = bNewDormant;
		SetActorHiddenInGame(bDormant);
		SetActorEnableCollision(!bDormant);

		UCharacterMovementComponent* Movement{GetCharacterMovement()};
		if (bDormant)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetDefaultMovementMode();
			if (MonsterController) MonsterController->ResetBehavior();
		}
	}

	// The State Tree can start after the monster registers, so stop it again on every call.
	if (bDormant && MonsterController) MonsterController->StopBehavior();
}

bool AMonsterCharacter::GrabPlayer(APlayerCharacter* Player)
{
	if (bGrabbing || bDormant || !IsValid(Player) || Player->IsPickedUp() || GetWorld()->GetTimeSeconds() < NextGrabTime)
	{
		UE_LOG(LogCotS, Log, TEXT("Monster %s: grab refused (grabbing %d dormant %d player %d pickedUp %d cooldown %.1f)"), *GetName(), bGrabbing, bDormant, IsValid(Player), IsValid(Player) && Player->IsPickedUp(), NextGrabTime - GetWorld()->GetTimeSeconds());
		return false;
	}
	UE_LOG(LogCotS, Log, TEXT("Monster %s: grab starts. AttackAnim %s, delay %.2f"), *GetName(), *GetNameSafe(AttackAnim), GrabDelaySeconds);

	EndStalk(false);
	GetWorldTimerManager().ClearTimer(StalkExitTimer);
	EndAttack(false);
	bGrabbing = true;
	GrabbedPlayer = Player;
	GetCharacterMovement()->StopMovementImmediately();

	const FVector ToPlayer{Player->GetActorLocation() - GetActorLocation()};
	SetActorRotation(FRotator{0.0, ToPlayer.Rotation().Yaw, 0.0});

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (AMonsterController* MonsterController{Cast<AMonsterController>(GetController())}) MonsterController->StopBehavior();
	}));

	if (UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()}; AnimInstance && AttackAnim)
	{
		const float Length{AnimInstance->Montage_Play(AttackAnim)};
		UE_LOG(LogCotS, Log, TEXT("Monster %s: attack montage %s plays, length %.2f"), *GetName(), *AttackAnim->GetName(), Length);
	}
	else
	{
		UE_LOG(LogCotS, Warning, TEXT("Monster %s: no attack montage plays. AttackAnim %s, anim instance %s"), *GetName(), *GetNameSafe(AttackAnim), *GetNameSafe(GetMesh()->GetAnimInstance()));
	}

	if (GrabDelaySeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(GrabTimer, this, &AMonsterCharacter::CompleteGrab, GrabDelaySeconds, false);
	}
	else
	{
		CompleteGrab();
	}
	return true;
}

void AMonsterCharacter::CompleteGrab()
{
	APlayerCharacter* Player{GrabbedPlayer.Get()};
	if (!bGrabbing || !IsValid(Player))
	{
		ReleasePlayer();
		return;
	}

	UE_LOG(LogCotS, Log, TEXT("Monster %s: grab completes. The player is picked up."), *GetName());
	Player->BeginPickedUp(this);
	PlayHoldLoop();
}

void AMonsterCharacter::PlayHoldLoop()
{
	UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()};
	if (!bGrabbing || !HoldingPlayerAnim || !AnimInstance)
	{
		UE_LOG(LogCotS, Warning, TEXT("Monster %s: no hold montage plays. HoldingPlayerAnim %s, anim instance %s"), *GetName(), *GetNameSafe(HoldingPlayerAnim), *GetNameSafe(AnimInstance));
		return;
	}

	const float HoldLength{AnimInstance->Montage_Play(HoldingPlayerAnim)};
	UE_LOG(LogCotS, Log, TEXT("Monster %s: hold montage %s plays, length %.2f"), *GetName(), *HoldingPlayerAnim->GetName(), HoldLength);

	if (HoldingPlayerAnim->GetNumSections() > 0)
	{
		const FName Section{HoldingPlayerAnim->GetSectionName(0)};
		AnimInstance->Montage_SetNextSection(Section, Section, HoldingPlayerAnim);
	}

	FOnMontageEnded EndDelegate{};
	EndDelegate.BindWeakLambda(this, [this](UAnimMontage*, const bool bInterrupted)
	{
		if (!bInterrupted) PlayHoldLoop();
	});
	AnimInstance->Montage_SetEndDelegate(EndDelegate, HoldingPlayerAnim);
}

void AMonsterCharacter::ReleasePlayer()
{
	if (!bGrabbing) return;

	UE_LOG(LogCotS, Log, TEXT("Monster %s: releases the player. DropAnim %s"), *GetName(), *GetNameSafe(DropAnim));
	bGrabbing = false;
	GrabbedPlayer.Reset();
	GetWorldTimerManager().ClearTimer(GrabTimer);

	float DropSeconds{};
	if (UAnimInstance* AnimInstance{GetMesh()->GetAnimInstance()})
	{
		if (HoldingPlayerAnim) AnimInstance->Montage_Stop(0.2f, HoldingPlayerAnim);
		if (AttackAnim) AnimInstance->Montage_Stop(0.2f, AttackAnim);
		if (DropAnim) DropSeconds = AnimInstance->Montage_Play(DropAnim);
	}

	NextGrabTime = GetWorld()->GetTimeSeconds() + StunSecondsAfterEscape + DropSeconds + FleeTimeoutSeconds;
	GetWorldTimerManager().SetTimer(StunTimer, this, &AMonsterCharacter::EndStun, FMath::Max(DropSeconds, 0.01f), false);
}

void AMonsterCharacter::EndStun()
{
	if (bDormant || bGrabbing) return;

	StartFlee();
}

void AMonsterCharacter::StartFlee()
{
	AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};
	if (!MonsterController || !FindFleeLocation(FleeLocation))
	{
		UE_LOG(LogCotS, Warning, TEXT("Monster %s: found no place to flee to"), *GetName());
		if (MonsterController) MonsterController->ResetBehavior();
		return;
	}

	UE_LOG(LogCotS, Log, TEXT("Monster %s: flees to %s"), *GetName(), *FleeLocation.ToString());
	bFleeing = true;
	FleeTimeLeft = FleeTimeoutSeconds;
	UCharacterMovementComponent* Movement{GetCharacterMovement()};
	SavedWalkSpeed = Movement->MaxWalkSpeed;
	if (FleeSpeed > 0.f) Movement->MaxWalkSpeed = FleeSpeed;
	MonsterController->MoveToLocation(FleeLocation, 100.f, true, true, true);
	SetActorTickEnabled(true);
}

void AMonsterCharacter::EndFlee()
{
	if (!bFleeing) return;

	UE_LOG(LogCotS, Log, TEXT("Monster %s: flee ends"), *GetName());
	bFleeing = false;
	SetActorTickEnabled(bAttacking || bStalking);
	GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;

	AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};
	if (!MonsterController) return;

	MonsterController->StopMovement();
	if (!bDormant && !bGrabbing) MonsterController->ResetBehavior();
}

bool AMonsterCharacter::OrderAttack(APlayerCharacter* Player)
{
	if (bAttacking || bGrabbing || bDormant || !IsValid(Player) || Player->IsPickedUp()) return false;
	if (GetWorld()->GetTimeSeconds() < NextGrabTime) return false;

	AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};
	if (!MonsterController) return false;

	UE_LOG(LogCotS, Log, TEXT("Monster %s: attack order accepted"), *GetName());
	EndStalk(false);
	GetWorldTimerManager().ClearTimer(StalkExitTimer);
	bAttacking = true;
	AttackTimeLeft = AttackTimeoutSeconds;
	AttackRepathTime = 0.f;
	MonsterController->StopBehavior();
	SetActorTickEnabled(true);
	return true;
}

void AMonsterCharacter::EndAttack(const bool bResumeBehavior)
{
	if (!bAttacking) return;

	UE_LOG(LogCotS, Log, TEXT("Monster %s: attack ends (resume %d)"), *GetName(), bResumeBehavior);
	bAttacking = false;
	SetActorTickEnabled(false);
	if (AMonsterController* MonsterController{Cast<AMonsterController>(GetController())})
	{
		MonsterController->StopMovement();
		if (bResumeBehavior && !bDormant && !bGrabbing) MonsterController->ResetBehavior();
	}
}

void AMonsterCharacter::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bFleeing)
	{
		FleeTimeLeft -= DeltaTime;
		const bool bArrived{FVector::Dist2D(GetActorLocation(), FleeLocation) <= 200.f};
		if (bArrived || FleeTimeLeft <= 0.f || bDormant) EndFlee();
		return;
	}

	if (bStalking)
	{
		const APlayerCharacter* StalkTarget{Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))};
		if (!StalkTarget || bDormant || bGrabbing || StalkTarget->IsPickedUp())
		{
			EndStalk(true);
			return;
		}

		const FRotator Target{0.0, (StalkTarget->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.0};
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Target, DeltaTime, StalkTurnSpeed));
		return;
	}

	if (!bAttacking) return;

	APlayerCharacter* Player{Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))};
	AAIController* AIController{Cast<AAIController>(GetController())};
	AttackTimeLeft -= DeltaTime;
	if (!Player || !AIController || bDormant || Player->IsPickedUp() || AttackTimeLeft <= 0.f)
	{
		EndAttack(true);
		return;
	}

	if (FVector::Dist(GetActorLocation(), Player->GetActorLocation()) <= AttackLungeDistance)
	{
		GrabPlayer(Player);
		return;
	}

	AttackRepathTime -= DeltaTime;
	if (AttackRepathTime <= 0.f)
	{
		AttackRepathTime = 0.4f;
		AIController->MoveToActor(Player, AttackLungeDistance * 0.5f, true, true, true);
	}
}

bool AMonsterCharacter::BeginStalk(APlayerCharacter* Player)
{
	if (bStalking) return true;
	if (bAttacking || bGrabbing || bFleeing || bDormant || !IsValid(Player) || Player->IsPickedUp()) return false;
	if (GetWorld()->GetTimeSeconds() < NextGrabTime) return false;

	AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};
	if (!MonsterController) return false;

	UE_LOG(LogCotS, Log, TEXT("Monster %s: stalks the player"), *GetName());
	GetWorldTimerManager().ClearTimer(StalkExitTimer);
	bStalking = true;
	MonsterController->StopBehavior();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorTickEnabled(true);
	return true;
}

void AMonsterCharacter::EndStalk(const bool bResumeBehavior)
{
	if (!bStalking) return;

	UE_LOG(LogCotS, Log, TEXT("Monster %s: stalk ends (resume %d)"), *GetName(), bResumeBehavior);
	bStalking = false;
	SetActorTickEnabled(bAttacking || bFleeing);

	if (!bResumeBehavior) return;

	GetWorldTimerManager().SetTimer(StalkExitTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (bStalking || bDormant || bGrabbing || bAttacking) return;
		if (AMonsterController* MonsterController{Cast<AMonsterController>(GetController())}) MonsterController->ResetBehavior();
	}), FMath::Max(StalkExitSeconds, 0.01f), false);
}

const TCHAR* AMonsterCharacter::GetModeLabel() const
{
	if (bDormant) return TEXT("DORMANT");
	if (bGrabbing) return TEXT("GRABBING");
	if (bAttacking) return TEXT("ATTACKING");
	if (bFleeing) return TEXT("FLEEING");
	if (bStalking) return TEXT("STALKING");
	if (GetWorldTimerManager().IsTimerActive(StalkExitTimer)) return TEXT("STALK EXIT");
	if (GetWorldTimerManager().IsTimerActive(StunTimer)) return TEXT("DROPPING");
	return TEXT("STATE TREE");
}

bool AMonsterCharacter::IsFreezeSuspended() const
{
	return bStalking || bAttacking || bGrabbing || bFleeing || GetWorldTimerManager().IsTimerActive(StalkExitTimer) || GetWorldTimerManager().IsTimerActive(StunTimer);
}

FVector AMonsterCharacter::GetHoldLocation() const
{
	const USkeletalMeshComponent* MeshComponent{GetMesh()};
	const FVector First{MeshComponent->GetSocketLocation(HoldSocketName)};
	if (SecondHoldSocketName.IsNone() || !MeshComponent->DoesSocketExist(SecondHoldSocketName)) return First;

	return (First + MeshComponent->GetSocketLocation(SecondHoldSocketName)) * 0.5;
}

bool AMonsterCharacter::IsStalking() const
{
	return bStalking || bAttacking || bGrabbing || bFleeing || GetWorldTimerManager().IsTimerActive(StunTimer);
}

bool AMonsterCharacter::FindFleeLocation(FVector& OutLocation) const
{
	const APawn* Player{UGameplayStatics::GetPlayerPawn(this, 0)};
	const FVector Self{GetActorLocation()};
	const FVector PlayerLocation{Player ? Player->GetActorLocation() : Self};

	if (const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)})
	{
		FVector SpawnLocation{};
		if (Director->GetFurthestSpawnLocation(PlayerLocation, SpawnLocation) && FVector::Dist2D(SpawnLocation, PlayerLocation) >= MinFleeDistance)
		{
			OutLocation = SpawnLocation;
			return true;
		}
	}

	UNavigationSystemV1* Navigation{FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())};
	if (!Navigation) return false;

	FVector Away{(Self - PlayerLocation).GetSafeNormal2D()};
	if (Away.IsNearlyZero()) Away = -GetActorForwardVector();
	const FVector Target{Self + Away * FleeDistance};

	FNavLocation Projected{};
	if (Navigation->ProjectPointToNavigation(Target, Projected, FVector{500.0, 500.0, 300.0}))
	{
		OutLocation = Projected.Location;
		return true;
	}
	if (Navigation->GetRandomReachablePointInRadius(Target, FleeDistance * 0.5f, Projected))
	{
		OutLocation = Projected.Location;
		return true;
	}
	return false;
}
