#include "CotSHeartbeat.h"

void FCotSHeartbeatState::AddStress(const float Amount)
{
	Stress = FMath::Clamp(Stress + Amount, 0.f, 1.f);
}

float CotSHeartbeat::MonsterStress(const float Distance, const float NearDistance, const float FarDistance)
{
	if (FarDistance <= NearDistance) return Distance <= NearDistance ? 1.f : 0.f;

	const float Part{FMath::Clamp((Distance - NearDistance) / (FarDistance - NearDistance), 0.f, 1.f)};
	return 1.f - Part;
}

float CotSHeartbeat::TargetStress(const FCotSHeartbeatSettings& Settings, const FCotSHeartbeatSenses& Senses)
{
	const float Run{Senses.bRunning ? Settings.RunStress : 0.f};
	const float Tired{Settings.TirednessStress * FMath::Clamp(Senses.Tiredness, 0.f, 1.f)};
	const float Monster{Settings.MonsterStress * MonsterStress(Senses.MonsterDistance, Settings.MonsterNearDistance, Settings.MonsterFarDistance)};

	// The strongest sense sets the rate. Two senses together do not push it over the top rate.
	float Target{FMath::Max3(Run, Tired, Monster)};
	if (Senses.bCalming) Target *= Settings.CalmScale;

	return FMath::Clamp(Target, 0.f, 1.f);
}

float CotSHeartbeat::BeatsPerMinute(const FCotSHeartbeatSettings& Settings, const float Stress)
{
	const float Fast{FMath::Max(Settings.FastBeatsPerMinute, Settings.CalmBeatsPerMinute)};
	const float Rate{FMath::Lerp(Settings.CalmBeatsPerMinute, Fast, FMath::Clamp(Stress, 0.f, 1.f))};
	return FMath::Max(Rate, 1.f);
}

float CotSHeartbeat::BeatInterval(const FCotSHeartbeatSettings& Settings, const float Stress)
{
	return 60.f / BeatsPerMinute(Settings, Stress);
}

float CotSHeartbeat::HintStrength(const FCotSHeartbeatSettings& Settings, const float Stress)
{
	return FMath::Max(FMath::Lerp(Settings.CalmHintStrength, Settings.LoudHintStrength, FMath::Clamp(Stress, 0.f, 1.f)), 0.f);
}

float CotSHeartbeat::PulseStrength(const FCotSHeartbeatSettings& Settings, const float Stress)
{
	return FMath::Clamp(FMath::Lerp(Settings.CalmPulse, Settings.LoudPulse, FMath::Clamp(Stress, 0.f, 1.f)), 0.f, 1.f);
}

float CotSHeartbeat::RevealRadius(const FCotSHeartbeatSettings& Settings, const float Stress)
{
	return FMath::Max(FMath::Lerp(Settings.CalmRevealRadius, Settings.LoudRevealRadius, FMath::Clamp(Stress, 0.f, 1.f)), 0.f);
}

bool CotSHeartbeat::ShouldRevealPulse(const FCotSHeartbeatSettings& Settings, const float TimeSinceLastRevealPulse)
{
	return TimeSinceLastRevealPulse >= FMath::Max(Settings.MinRevealPulseSeconds, 0.f);
}

float CotSHeartbeat::RevealPulseRiseSeconds(const FCotSHeartbeatSettings& Settings, const float Stress)
{
	return FMath::Max(BeatInterval(Settings, Stress), Settings.MinRevealPulseSeconds);
}

int32 CotSHeartbeat::Step(const FCotSHeartbeatSettings& Settings, const FCotSHeartbeatSenses& Senses, FCotSHeartbeatState& State, const float DeltaTime)
{
	if (DeltaTime <= 0.f) return 0;

	const float Target{TargetStress(Settings, Senses)};
	const float Speed{Target > State.Stress ? Settings.StressRisePerSecond : Settings.StressFallPerSecond};
	State.Stress = FMath::Clamp(FMath::FInterpConstantTo(State.Stress, Target, DeltaTime, FMath::Max(Speed, KINDA_SMALL_NUMBER)), 0.f, 1.f);

	State.Pulse = FMath::Max(State.Pulse - DeltaTime / FMath::Max(Settings.PulseFallSeconds, KINDA_SMALL_NUMBER), 0.f);

	const float Interval{BeatInterval(Settings, State.Stress)};
	const int32 MaxBeats{FMath::Max(Settings.MaxBeatsPerStep, 1)};

	State.TimeToNextBeat -= DeltaTime;
	int32 Beats{};
	while (State.TimeToNextBeat <= 0.f && Beats < MaxBeats)
	{
		++Beats;
		State.TimeToNextBeat += Interval;
	}

	// A long frame leaves a backlog of beats. The heart drops it and waits one full interval.
	if (State.TimeToNextBeat <= 0.f) State.TimeToNextBeat = Interval;

	if (Beats > 0) State.Pulse = PulseStrength(Settings, State.Stress);

	return Beats;
}
