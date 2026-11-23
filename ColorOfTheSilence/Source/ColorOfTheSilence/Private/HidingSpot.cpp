#include "HidingSpot.h"

#include "CotSObjectiveSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "SectionVolume.h"

AHidingSpot::AHidingSpot()
{
	SpotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpotRoot"));
	SetRootComponent(SpotRoot);

	InspectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("InspectPoint"));
	InspectPoint->SetupAttachment(SpotRoot);
	InspectPoint->SetRelativeLocation(FVector(100.f, 0.f, 0.f));
}

void AHidingSpot::BeginPlay()
{
	Super::BeginPlay();

	ASectionVolume::AssignSectionFromVolume(*this, Section);

	if (UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)}) Subsystem->RegisterHidingSpot(this);
}

void AHidingSpot::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UCotSObjectiveSubsystem* Subsystem{UCotSObjectiveSubsystem::Get(this)}) Subsystem->UnregisterHidingSpot(this);

	Super::EndPlay(Reason);
}

FVector AHidingSpot::GetInspectLocation() const
{
	return InspectPoint->GetComponentLocation();
}

float AHidingSpot::GetSecondsSinceChecked() const
{
	const UWorld* World{GetWorld()};
	if (LastCheckedTime < 0.0 || !World) return TNumericLimits<float>::Max();

	return static_cast<float>(World->GetTimeSeconds() - LastCheckedTime);
}

void AHidingSpot::MarkChecked()
{
	if (const UWorld* World{GetWorld()}) LastCheckedTime = World->GetTimeSeconds();
}
