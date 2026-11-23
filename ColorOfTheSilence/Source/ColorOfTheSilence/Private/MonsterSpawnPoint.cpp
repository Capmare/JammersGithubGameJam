#include "MonsterSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "Engine/World.h"
#include "SectionVolume.h"

AMonsterSpawnPoint::AMonsterSpawnPoint()
{
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	SetRootComponent(Arrow);
	Arrow->SetHiddenInGame(true);
}

void AMonsterSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	ASectionVolume::AssignSectionFromVolume(*this, Section);

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->RegisterSpawnPoint(this);
}

void AMonsterSpawnPoint::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->UnregisterSpawnPoint(this);

	Super::EndPlay(Reason);
}
