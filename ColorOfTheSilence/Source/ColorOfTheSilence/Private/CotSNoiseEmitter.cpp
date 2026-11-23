#include "CotSNoiseEmitter.h"

#include "Components/SphereComponent.h"
#include "CotSMonsterDirector.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACotSNoiseEmitter::ACotSNoiseEmitter()
{
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetSphereRadius(150.f);
	Trigger->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	Trigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Trigger->SetGenerateOverlapEvents(true);
}

void ACotSNoiseEmitter::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACotSNoiseEmitter::HandleBeginOverlap);

	if (bEmitOnTimer) GetWorldTimerManager().SetTimer(TimerHandle, this, &ACotSNoiseEmitter::EmitAndSchedule, FMath::Max(StartDelay, 0.01f), false);
}

void ACotSNoiseEmitter::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(TimerHandle);
	Super::EndPlay(Reason);
}

void ACotSNoiseEmitter::EmitNoise()
{
	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->GiveHint(GetActorLocation(), Loudness, Source);
}

void ACotSNoiseEmitter::EmitAndSchedule()
{
	EmitNoise();

	const float Wait{FMath::Max(Interval + FMath::FRandRange(-IntervalJitter, IntervalJitter), 0.1f)};
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ACotSNoiseEmitter::EmitAndSchedule, Wait, false);
}

void ACotSNoiseEmitter::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bEmitOnPlayerEnter || (bTriggerOnce && bTriggered)) return;
	if (!OtherActor || OtherActor != UGameplayStatics::GetPlayerPawn(this, 0)) return;

	bTriggered = true;
	EmitNoise();
}
