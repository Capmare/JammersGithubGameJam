#include "CotSHeartbeatComponent.h"

#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "../PaintEffectComponent.h"
#include "../PlayerCharacter.h"

UCotSHeartbeatComponent::UCotSHeartbeatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCotSHeartbeatComponent::BeginPlay()
{
	Super::BeginPlay();

	State = FCotSHeartbeatState{};
	State.TimeToNextBeat = CotSHeartbeat::BeatInterval(Settings, State.Stress);
	// Large enough that the very first beat is always allowed to open a reveal pulse right away.
	TimeSinceLastRevealPulse = Settings.MinRevealPulseSeconds;

	if (const AActor* Owner{GetOwner()})
	{
		Paint = Owner->FindComponentByClass<UPaintEffectComponent>();
	}

	if (!Cast<APlayerCharacter>(GetOwner()))
	{
		UE_LOG(LogCotS, Warning, TEXT("Heartbeat: %s is not a player character. The heart does not feel the run or the tiredness."), *GetNameSafe(GetOwner()));
	}
	if (bPulseReveal && !Paint)
	{
		UE_LOG(LogCotS, Warning, TEXT("Heartbeat: %s has no paint effect component. The beats show no more of the world."), *GetNameSafe(GetOwner()));
	}
}

void UCotSHeartbeatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// The stress also grows outside the sound world. The heart then already pounds when the sound world comes back.
	const int32 Beats{CotSHeartbeat::Step(Settings, ReadSenses(), State, DeltaTime)};
	TimeSinceLastRevealPulse += DeltaTime;

	bBeating = !bOnlyInSoundWorld || IsInSoundWorld();
	if (!bBeating)
	{
		State.Pulse = 0.f;
		return;
	}

	for (int32 Index{}; Index < Beats; ++Index) Beat();
}

FCotSHeartbeatSenses UCotSHeartbeatComponent::ReadSenses() const
{
	FCotSHeartbeatSenses Senses{};

	const AActor* Owner{GetOwner()};
	if (!Owner) return Senses;

	if (const APlayerCharacter* Player{Cast<APlayerCharacter>(Owner)})
	{
		Senses.bRunning = Player->IsRunning();
		Senses.bCalming = Player->IsCalming();
		Senses.Tiredness = Player->GetTiredness();
	}

	if (const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)})
	{
		Senses.MonsterDistance = Director->GetNearestMonsterDistance(Owner->GetActorLocation());
	}

	return Senses;
}

bool UCotSHeartbeatComponent::IsInSoundWorld() const
{
	if (const APlayerCharacter* Player{Cast<APlayerCharacter>(GetOwner())}) return Player->IsInSoundWorld();

	const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)};
	return Director && Director->IsSoundWorldActive();
}

void UCotSHeartbeatComponent::Beat()
{
	if (BeatSound) UGameplayStatics::PlaySound2D(this, BeatSound);

	if (const AActor* Owner{GetOwner()}; bSendHints && Owner)
	{
		const float Strength{CotSHeartbeat::HintStrength(Settings, State.Stress)};
		UCotSMonsterDirector::SendHint(this, Owner->GetActorLocation(), Strength, HintSource);
	}

	// A hint goes out on every beat, but the reveal pulse itself is throttled: a racing heart beats
	// faster than Settings.MinRevealPulseSeconds, and opening a new ripple on every single one of
	// those beats would flash the screen far too fast to look at. Skipped beats still count, just
	// silently, toward the next allowed pulse.
	if (bPulseReveal && IsValid(Paint) && CotSHeartbeat::ShouldRevealPulse(Settings, TimeSinceLastRevealPulse))
	{
		Paint->PulseRadius(CotSHeartbeat::RevealRadius(Settings, State.Stress), CotSHeartbeat::RevealPulseRiseSeconds(Settings, State.Stress));
		TimeSinceLastRevealPulse = 0.f;
	}

	OnHeartbeat.Broadcast(State.Pulse, State.Stress);
}
