#include "SectionVolume.h"

#include "Components/BoxComponent.h"
#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

ASectionVolume::ASectionVolume()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(500.f));
	Box->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Box->SetGenerateOverlapEvents(true);
}

void ASectionVolume::BeginPlay()
{
	Super::BeginPlay();

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->RegisterSectionVolume(this);

	Box->OnComponentBeginOverlap.AddDynamic(this, &ASectionVolume::HandleBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &ASectionVolume::HandleEndOverlap);

	TArray<AActor*> Overlapping;
	Box->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (const APawn* Pawn{Cast<APawn>(Actor)}; Pawn && Pawn->IsPlayerControlled())
		{
			if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->SetPlayerSection(Section);
		}
	}
}

void ASectionVolume::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->UnregisterSectionVolume(this);

	Super::EndPlay(Reason);
}

bool ASectionVolume::ContainsPoint(const FVector& WorldPoint) const
{
	const FVector Local{Box->GetComponentTransform().InverseTransformPositionNoScale(WorldPoint)};
	const FVector Extent{Box->GetScaledBoxExtent()};
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

double ASectionVolume::GetVerticalGap(const FVector& WorldPoint) const
{
	const FVector Local{Box->GetComponentTransform().InverseTransformPositionNoScale(WorldPoint)};
	const FVector Extent{Box->GetScaledBoxExtent()};
	if (FMath::Abs(Local.X) > Extent.X || FMath::Abs(Local.Y) > Extent.Y) return TNumericLimits<double>::Max();

	return FMath::Max(0.0, FMath::Abs(Local.Z) - Extent.Z);
}

double ASectionVolume::GetDistanceToBox(const FVector& WorldPoint) const
{
	const FVector Local{Box->GetComponentTransform().InverseTransformPositionNoScale(WorldPoint)};
	const FVector Extent{Box->GetScaledBoxExtent()};
	return FVector(FMath::Max(FMath::Abs(Local.X) - Extent.X, 0.0), FMath::Max(FMath::Abs(Local.Y) - Extent.Y, 0.0), FMath::Max(FMath::Abs(Local.Z) - Extent.Z, 0.0)).Size();
}

void ASectionVolume::AssignSectionFromVolume(const AActor& Actor, int32& InOutSection)
{
	const UWorld* World{Actor.GetWorld()};
	if (!World) return;

	const FVector Location{Actor.GetActorLocation()};
	if (const int32 Found{FindSectionAt(World, Location)}; Found != INDEX_NONE)
	{
		InOutSection = Found;
		return;
	}

	const ASectionVolume* Nearest{};
	double NearestDistance{TNumericLimits<double>::Max()};
	for (TActorIterator<ASectionVolume> It(World); It; ++It)
	{
		if (const double Distance{It->GetDistanceToBox(Location)}; Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = *It;
		}
	}

	if (!Nearest)
	{
		UE_LOG(LogCotS, Warning, TEXT("'%s' found no section volume in the level. It keeps Section %d."), *Actor.GetName(), InOutSection);
		return;
	}

	InOutSection = Nearest->GetSection();
	UE_LOG(LogCotS, Warning, TEXT("'%s' at %s is outside every section volume. It uses section %d from the nearest volume '%s', %.0f units away."), *Actor.GetName(), *Location.ToString(), InOutSection, *Nearest->GetName(), NearestDistance);
}

int32 ASectionVolume::FindSectionAt(const UWorld* World, const FVector& WorldPoint)
{
	constexpr double MaxVerticalGap{400.0};

	if (!World) return INDEX_NONE;

	int32 Best{INDEX_NONE};
	double BestGap{MaxVerticalGap};
	for (TActorIterator<ASectionVolume> It(World); It; ++It)
	{
		const double Gap{It->GetVerticalGap(WorldPoint)};
		if (Gap <= BestGap)
		{
			BestGap = Gap;
			Best = It->GetSection();
		}
	}
	return Best;
}

void ASectionVolume::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	const APawn* Pawn{Cast<APawn>(OtherActor)};
	if (!Pawn || !Pawn->IsPlayerControlled()) return;

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->SetPlayerSection(Section);
}

void ASectionVolume::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	const APawn* Pawn{Cast<APawn>(OtherActor)};
	if (!Pawn || !Pawn->IsPlayerControlled()) return;

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->SetPlayerSection(FindSectionAt(GetWorld(), Pawn->GetActorLocation()));
}
