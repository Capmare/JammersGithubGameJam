#include "MonsterFreezeComponent.h"
#include "MonsterCharacter.h"

#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UMonsterFreezeComponent::UMonsterFreezeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UMonsterFreezeComponent::BeginPlay()
{
	Super::BeginPlay();

	const ACharacter* Character{Cast<ACharacter>(GetOwner())};
	if (!Character)
	{
		UE_LOG(LogCotS, Warning, TEXT("Freeze component on '%s' needs a Character owner. It is disabled."), *GetOwner()->GetName());
		SetComponentTickEnabled(false);
	}
}

void UMonsterFreezeComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bFrozen) SetFrozen(false);

	Super::EndPlay(Reason);
}

void UMonsterFreezeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bSeen = ComputeSeen();

	const AMonsterCharacter* Monster{Cast<AMonsterCharacter>(GetOwner())};
	if (Monster && Monster->IsFreezeSuspended())
	{
		UnseenTime = UnseenDelay;
		if (bFrozen) SetFrozen(false);
		return;
	}

	const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)};
	const bool bAware{!bFreezeOnlyWhenAware || (Director && Director->IsPlayerDetected())};

	if (bSeen && bAware)
	{
		UnseenTime = 0.f;
		if (!bFrozen) SetFrozen(true);
		return;
	}

	UnseenTime += DeltaTime;
	if (bFrozen && UnseenTime >= UnseenDelay) SetFrozen(false);
}

bool UMonsterFreezeComponent::ComputeSeen() const
{
	const AActor* Owner{GetOwner()};
	const APlayerCameraManager* Camera{UGameplayStatics::GetPlayerCameraManager(this, 0)};
	if (!Owner || !Camera) return false;

	const FVector EyePoint{Camera->GetCameraLocation()};
	const FVector Forward{Camera->GetCameraRotation().Vector()};
	const float HalfAngleCos{FMath::Cos(FMath::DegreesToRadians(Camera->GetFOVAngle() * 0.5f + ViewAngleMargin))};

	FVector Origin;
	FVector Extent;
	Owner->GetActorBounds(true, Origin, Extent);

	const FVector Points[]{
		Origin + FVector(0.f, 0.f, Extent.Z * 0.9f),
		Origin,
		Origin - FVector(0.f, 0.f, Extent.Z * 0.9f)};

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterSight), false);
	Params.AddIgnoredActor(Owner);
	Params.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(this, 0));

	for (const FVector& Point : Points)
	{
		const FVector ToPoint{Point - EyePoint};
		const float Distance{static_cast<float>(ToPoint.Size())};
		if (Distance < KINDA_SMALL_NUMBER) return true;
		if (MaxSeenDistance > 0.f && Distance > MaxSeenDistance) continue;
		if (FVector::DotProduct(ToPoint / Distance, Forward) < HalfAngleCos) continue;

		if (!GetWorld()->LineTraceTestByChannel(EyePoint, Point, SightChannel, Params)) return true;
	}

	return false;
}

void UMonsterFreezeComponent::SetFrozen(const bool bNewFrozen)
{
	if (bFrozen == bNewFrozen) return;
	bFrozen = bNewFrozen;

	if (ACharacter* Character{Cast<ACharacter>(GetOwner())})
	{
		if (UCharacterMovementComponent* Movement{Character->GetCharacterMovement()})
		{
			if (bFrozen)
			{
				SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
				bSavedOrientToMovement = Movement->bOrientRotationToMovement;
				bSavedDesiredRotation = Movement->bUseControllerDesiredRotation;
				bSavedControllerYaw = Character->bUseControllerRotationYaw;

				// The controller and the movement component turn the monster, so freeze them too.
				Movement->MaxWalkSpeed = 0.f;
				Movement->bOrientRotationToMovement = false;
				Movement->bUseControllerDesiredRotation = false;
				Character->bUseControllerRotationYaw = false;
				Movement->StopMovementImmediately();
			}
			else
			{
				Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
				Movement->bOrientRotationToMovement = bSavedOrientToMovement;
				Movement->bUseControllerDesiredRotation = bSavedDesiredRotation;
				Character->bUseControllerRotationYaw = bSavedControllerYaw;
			}
		}

		if (USkeletalMeshComponent* Mesh{Character->GetMesh()}) Mesh->bPauseAnims = bFrozen;
	}

	UE_LOG(LogCotS, Verbose, TEXT("Monster '%s' %s."), *GetOwner()->GetName(), bFrozen ? TEXT("freezes") : TEXT("moves again"));
	OnFreezeChanged.Broadcast(bFrozen);
}
