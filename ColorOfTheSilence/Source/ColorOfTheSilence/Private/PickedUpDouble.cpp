#include "PickedUpDouble.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "../PlayerCharacter.h"

APickedUpDouble::APickedUpDouble()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DoubleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DoubleMesh"));
	DoubleMesh->SetupAttachment(Root);
	DoubleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoubleMesh->SetGenerateOverlapEvents(false);
}

void APickedUpDouble::Initialize(const APlayerCharacter* Player)
{
	const USkeletalMeshComponent* Source{Player ? Player->GetMesh() : nullptr};
	if (!Source) return;

	DoubleMesh->SetSkeletalMeshAsset(Source->GetSkeletalMeshAsset());
	DoubleMesh->SetRelativeTransform(Source->GetRelativeTransform());
	for (int32 Index{0}; Index < Source->GetNumMaterials(); ++Index) DoubleMesh->SetMaterial(Index, Source->GetMaterial(Index));
	DoubleMesh->SetLeaderPoseComponent(const_cast<USkeletalMeshComponent*>(Source));
}

void APickedUpDouble::BeginBlend(USkeletalMeshComponent* Parent, const FName Socket, const FTransform& Relative, const float Seconds)
{
	BlendParent = Parent;
	BlendSocket = Socket;
	BlendRelative = Relative;
	BlendStart = GetActorTransform();
	BlendDuration = Seconds;
	BlendElapsed = 0.f;

	if (!Parent || Seconds <= 0.f)
	{
		FinishBlend();
		return;
	}
	SetActorTickEnabled(true);
}

void APickedUpDouble::FinishBlend()
{
	SetActorTickEnabled(false);
	if (USkeletalMeshComponent* Parent{BlendParent.Get()})
	{
		AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BlendSocket);
		SetActorRelativeTransform(BlendRelative);
	}
}

void APickedUpDouble::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	const USkeletalMeshComponent* Parent{BlendParent.Get()};
	if (!Parent)
	{
		SetActorTickEnabled(false);
		return;
	}

	BlendElapsed += DeltaTime;
	const float Alpha{FMath::SmoothStep(0.f, 1.f, BlendElapsed / BlendDuration)};
	const FTransform Target{BlendRelative * Parent->GetSocketTransform(BlendSocket)};

	FTransform Current{};
	Current.Blend(BlendStart, Target, Alpha);
	SetActorTransform(Current);

	if (BlendElapsed >= BlendDuration) FinishBlend();
}
