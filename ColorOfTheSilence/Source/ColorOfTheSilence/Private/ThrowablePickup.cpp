#include "ThrowablePickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CotSMonsterDirector.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameFramework/Pawn.h"

AThrowablePickup::AThrowablePickup()
{
	PhysicsBody = CreateDefaultSubobject<USphereComponent>(TEXT("PhysicsBody"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->InitSphereRadius(CollisionRadius);
	PhysicsBody->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	// Pawns pass through the object, so it never pushes or traps the player. The camera ignores it too
	PhysicsBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	// Physics starts only after a throw, so the object does not roll around in the level
	PhysicsBody->SetSimulatePhysics(false);
	PhysicsBody->SetNotifyRigidBodyCollision(true);

	// The mesh only shows the object. The sphere holds the physics, so the mesh needs no collision
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(PhysicsBody);
	VisualMesh->SetSimulatePhysics(false);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetMobility(EComponentMobility::Movable);
}

void AThrowablePickup::BeginPlay()
{
	Super::BeginPlay();

	// A Blueprint child can override the constructor values, so BeginPlay sets them again
	// The old Blueprint saved physics on the mesh. A simulating mesh breaks away from the sphere
	VisualMesh->SetSimulatePhysics(false);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (VisualMesh->GetAttachParent() != PhysicsBody)
	{
		VisualMesh->AttachToComponent(PhysicsBody, FAttachmentTransformRules::KeepRelativeTransform);
	}
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetSimulatePhysics(false);
	PhysicsBody->SetSphereRadius(CollisionRadius);
	PhysicsBody->SetMassOverrideInKg(NAME_None, MassKg);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// The Max combine mode makes the bounce count even when the floor has none
	UPhysicalMaterial* PhysMaterial{NewObject<UPhysicalMaterial>(this)};
	PhysMaterial->Restitution = Bounciness;
	PhysMaterial->Friction = Friction;
	PhysMaterial->bOverrideRestitutionCombineMode = true;
	PhysMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
	PhysicsBody->SetPhysMaterialOverride(PhysMaterial);

	if (bMeshFixed) return;

	TArray<UStaticMesh*> ValidMeshes;
	for (UStaticMesh* Candidate : RandomMeshes)
	{
		if (Candidate) ValidMeshes.Add(Candidate);
	}

	if (!ValidMeshes.IsEmpty())
	{
		// The cycle counter differs for each actor, so no two actors share a seed
		FRandomStream Stream{static_cast<int32>(FPlatformTime::Cycles64() ^ (static_cast<uint64>(GetUniqueID()) * 2654435761ull))};

		// A shared record of the last pick stops the same mesh from repeating at once
		static TWeakObjectPtr<UStaticMesh> LastPick{};
		TArray<UStaticMesh*> Others{ValidMeshes};
		Others.Remove(LastPick.Get());
		if (!Others.IsEmpty()) ValidMeshes = Others;

		UStaticMesh* Pick{ValidMeshes[Stream.RandRange(0, ValidMeshes.Num() - 1)]};
		LastPick = Pick;
		VisualMesh->SetStaticMesh(Pick);
	}
}

void AThrowablePickup::Launch(const FVector& Velocity)
{
	// A Blueprint child can override these values, so the throw sets them again
	PhysicsBody->SetMobility(EComponentMobility::Movable);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	bLaunched = true;
	GetWorldTimerManager().ClearTimer(FreezeTimer);
	PhysicsBody->SetEnableGravity(true);
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->WakeAllRigidBodies();
	PhysicsBody->SetPhysicsLinearVelocity(Velocity);
}

void AThrowablePickup::FreezePhysics()
{
	PhysicsBody->SetSimulatePhysics(false);
	bLaunched = false;
}

void AThrowablePickup::UseMesh(UStaticMesh* NewMesh)
{
	if (!NewMesh) return;

	bMeshFixed = true;
	VisualMesh->SetStaticMesh(NewMesh);
}

UStaticMesh* AThrowablePickup::GetCurrentMesh() const
{
	return VisualMesh->GetStaticMesh();
}

void AThrowablePickup::PickUp_Implementation(AActor* Interactor)
{
	if (bHeld || !Interactor) return;

	if (bPlayerOnly)
	{
		const APawn* InteractingPawn{Cast<APawn>(Interactor)};
		if (!InteractingPawn || !InteractingPawn->IsPlayerControlled()) return;
	}

	bHeld = true;
	bLaunched = false;
	GetWorldTimerManager().ClearTimer(FreezeTimer);
	PhysicsBody->SetSimulatePhysics(false);
	SetActorEnableCollision(false);
}

void AThrowablePickup::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, const bool bSelfMoved, const FVector HitLocation, const FVector HitNormal, const FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// The first ground hit starts a short timer. When it ends, the physics stops
	const bool bGround{FMath::Abs(Hit.ImpactNormal.Z) >= GroundNormalMinZ};
	if (bLaunched && bGround && !GetWorldTimerManager().IsTimerActive(FreezeTimer))
	{
		GetWorldTimerManager().SetTimer(FreezeTimer, this, &AThrowablePickup::FreezePhysics, PhysicsTimeAfterLanding);
	}

	const bool bFirstHit{bThrownPending};
	bThrownPending = false;

	if (ImpactHintStrength <= 0.f) return;

	const UWorld* World{GetWorld()};
	if (!World) return;

	const double Now{World->GetTimeSeconds()};
	if (!bFirstHit && (NormalImpulse.Size() < MinImpactImpulse || Now - LastHintTime < HintCooldown)) return;

	LastHintTime = Now;
	UCotSMonsterDirector::SendHint(this, HitLocation, ImpactHintStrength, TEXT("ThrowableImpact"));
}
