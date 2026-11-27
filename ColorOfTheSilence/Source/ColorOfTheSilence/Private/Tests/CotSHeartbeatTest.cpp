#include "CotSHeartbeat.h"
#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

namespace
{
constexpr EAutomationTestFlags HeartbeatTestFlags{EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter};
constexpr float TestStep{1.f / 60.f};

// No monster in range.
constexpr float NoMonster{1.0e9f};

FCotSHeartbeatSenses CalmSenses()
{
	return FCotSHeartbeatSenses{NoMonster, 0.f, false, false};
}

FCotSHeartbeatSenses RunSenses()
{
	FCotSHeartbeatSenses Senses{CalmSenses()};
	Senses.bRunning = true;
	return Senses;
}

FCotSHeartbeatSenses MonsterSenses(const float Distance)
{
	FCotSHeartbeatSenses Senses{CalmSenses()};
	Senses.MonsterDistance = Distance;
	return Senses;
}

// Runs the heart for Seconds and gives back the number of beats.
int32 CountBeats(const FCotSHeartbeatSettings& Settings, const FCotSHeartbeatSenses& Senses, FCotSHeartbeatState& State, const float Seconds)
{
	int32 Beats{};
	for (float Passed{}; Passed < Seconds; Passed += TestStep)
	{
		Beats += CotSHeartbeat::Step(Settings, Senses, State, TestStep);
	}
	return Beats;
}

// Brings the heart to the stress that the senses ask for.
void Settle(const FCotSHeartbeatSettings& Settings, const FCotSHeartbeatSenses& Senses, FCotSHeartbeatState& State)
{
	CountBeats(Settings, Senses, State, 30.f);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatMonsterStressTest, "CotS.Heartbeat.MonsterStressFallsWithDistance", HeartbeatTestFlags)
bool FCotSHeartbeatMonsterStressTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A monster on top of the player gives full stress"), CotSHeartbeat::MonsterStress(0.f, 600.f, 2500.f), 1.f);
	TestEqual(TEXT("A monster at the near distance gives full stress"), CotSHeartbeat::MonsterStress(600.f, 600.f, 2500.f), 1.f);
	TestEqual(TEXT("A monster at the far distance gives no stress"), CotSHeartbeat::MonsterStress(2500.f, 600.f, 2500.f), 0.f);
	TestEqual(TEXT("A monster behind the far distance gives no stress"), CotSHeartbeat::MonsterStress(9000.f, 600.f, 2500.f), 0.f);
	TestEqual(TEXT("No monster gives no stress"), CotSHeartbeat::MonsterStress(NoMonster, 600.f, 2500.f), 0.f);
	TestNearlyEqual(TEXT("A monster in the middle gives half stress"), CotSHeartbeat::MonsterStress(1550.f, 600.f, 2500.f), 0.5f, 0.01f);

	// A designer can put the two distances in the wrong order. The stress stays in range.
	TestEqual(TEXT("A far distance below the near distance still works"), CotSHeartbeat::MonsterStress(100.f, 600.f, 100.f), 1.f);
	TestEqual(TEXT("A far distance below the near distance gives no stress far away"), CotSHeartbeat::MonsterStress(5000.f, 600.f, 100.f), 0.f);

	const float Closer{CotSHeartbeat::MonsterStress(800.f, 600.f, 2500.f)};
	const float Further{CotSHeartbeat::MonsterStress(2000.f, 600.f, 2500.f)};
	TestTrue(TEXT("A closer monster gives more stress"), Closer > Further);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatRunRaisesRateTest, "CotS.Heartbeat.RunningRaisesTheRate", HeartbeatTestFlags)
bool FCotSHeartbeatRunRaisesRateTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState Idle{};
	Settle(Settings, CalmSenses(), Idle);

	FCotSHeartbeatState Running{};
	Settle(Settings, RunSenses(), Running);

	TestTrue(TEXT("A run raises the stress"), Running.Stress > Idle.Stress);
	TestTrue(TEXT("A run raises the rate"), CotSHeartbeat::BeatsPerMinute(Settings, Running.Stress) > CotSHeartbeat::BeatsPerMinute(Settings, Idle.Stress));
	TestTrue(TEXT("A run shortens the time between two beats"), CotSHeartbeat::BeatInterval(Settings, Running.Stress) < CotSHeartbeat::BeatInterval(Settings, Idle.Stress));
	TestNearlyEqual(TEXT("A run holds the stress at RunStress"), Running.Stress, Settings.RunStress, 0.01f);
	TestNearlyEqual(TEXT("A calm player sits at the calm rate"), CotSHeartbeat::BeatsPerMinute(Settings, Idle.Stress), Settings.CalmBeatsPerMinute, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatMonsterRaisesRateTest, "CotS.Heartbeat.MonsterCloseRaisesTheRate", HeartbeatTestFlags)
bool FCotSHeartbeatMonsterRaisesRateTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState Close{};
	Settle(Settings, MonsterSenses(300.f), Close);

	FCotSHeartbeatState Far{};
	Settle(Settings, MonsterSenses(2000.f), Far);

	FCotSHeartbeatState Away{};
	Settle(Settings, MonsterSenses(NoMonster), Away);

	TestTrue(TEXT("A close monster gives more stress than a far monster"), Close.Stress > Far.Stress);
	TestTrue(TEXT("A far monster gives more stress than no monster"), Far.Stress > Away.Stress);
	TestTrue(TEXT("A close monster shortens the time between two beats"), CotSHeartbeat::BeatInterval(Settings, Close.Stress) < CotSHeartbeat::BeatInterval(Settings, Far.Stress));
	TestNearlyEqual(TEXT("A close monster gives full stress"), Close.Stress, 1.f, 0.01f);
	TestNearlyEqual(TEXT("A close monster gives the fast rate"), CotSHeartbeat::BeatsPerMinute(Settings, Close.Stress), Settings.FastBeatsPerMinute, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatMoreHintsTest, "CotS.Heartbeat.FastHeartSendsMoreHints", HeartbeatTestFlags)
bool FCotSHeartbeatMoreHintsTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};
	constexpr float Window{10.f};

	FCotSHeartbeatState Calm{};
	Settle(Settings, CalmSenses(), Calm);
	const int32 CalmBeats{CountBeats(Settings, CalmSenses(), Calm, Window)};

	FCotSHeartbeatState Running{};
	Settle(Settings, RunSenses(), Running);
	const int32 RunBeats{CountBeats(Settings, RunSenses(), Running, Window)};

	FCotSHeartbeatState Hunted{};
	Settle(Settings, MonsterSenses(200.f), Hunted);
	const int32 HuntedBeats{CountBeats(Settings, MonsterSenses(200.f), Hunted, Window)};

	// One beat is one hint, so more beats are more hints.
	TestTrue(TEXT("A calm heart beats"), CalmBeats > 0);
	TestTrue(TEXT("A running player sends more hints than a calm player"), RunBeats > CalmBeats);
	TestTrue(TEXT("A hunted player sends more hints than a running player"), HuntedBeats > RunBeats);

	// 60 beats per minute is 10 beats in 10 seconds.
	TestEqual(TEXT("A calm heart beats one time each second"), CalmBeats, 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatCalmsDownTest, "CotS.Heartbeat.HeartCalmsDownOverTime", HeartbeatTestFlags)
bool FCotSHeartbeatCalmsDownTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState State{};
	Settle(Settings, MonsterSenses(100.f), State);
	TestNearlyEqual(TEXT("The monster brings the heart to full stress"), State.Stress, 1.f, 0.01f);

	// The monster leaves and the player stands still.
	const float HalfWay{1.f / (2.f * Settings.StressFallPerSecond)};
	CountBeats(Settings, CalmSenses(), State, HalfWay);
	TestTrue(TEXT("The heart is still above the calm rate half way down"), State.Stress > 0.1f);
	TestTrue(TEXT("The heart drops below full stress half way down"), State.Stress < 0.9f);

	CountBeats(Settings, CalmSenses(), State, HalfWay + 1.f);
	TestEqual(TEXT("The heart is fully calm in the end"), State.Stress, 0.f);
	TestNearlyEqual(TEXT("The heart is back at the calm rate"), CotSHeartbeat::BeatsPerMinute(Settings, State.Stress), Settings.CalmBeatsPerMinute, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatRiseIsNotInstantTest, "CotS.Heartbeat.StressRiseIsNotInstant", HeartbeatTestFlags)
bool FCotSHeartbeatRiseIsNotInstantTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState State{};
	CountBeats(Settings, MonsterSenses(100.f), State, 0.2f);

	TestTrue(TEXT("The heart starts to rise"), State.Stress > 0.f);
	TestTrue(TEXT("The heart does not jump to full stress"), State.Stress < 1.f);

	// A rise of 0.8 per second needs 1.25 seconds, so 2 seconds is enough.
	CountBeats(Settings, MonsterSenses(100.f), State, 2.f);
	TestEqual(TEXT("The heart reaches full stress"), State.Stress, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatCalmKeyTest, "CotS.Heartbeat.CalmingLowersTheRate", HeartbeatTestFlags)
bool FCotSHeartbeatCalmKeyTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatSenses Hunted{MonsterSenses(100.f)};
	FCotSHeartbeatState Loud{};
	Settle(Settings, Hunted, Loud);

	FCotSHeartbeatSenses HuntedAndCalming{Hunted};
	HuntedAndCalming.bCalming = true;
	FCotSHeartbeatState Quiet{};
	Settle(Settings, HuntedAndCalming, Quiet);

	TestTrue(TEXT("The calm key lowers the stress"), Quiet.Stress < Loud.Stress);
	TestNearlyEqual(TEXT("The calm key scales the stress"), Quiet.Stress, Settings.CalmScale, 0.01f);
	TestTrue(TEXT("The calm key lowers the number of hints"), CotSHeartbeat::HintStrength(Settings, Quiet.Stress) < CotSHeartbeat::HintStrength(Settings, Loud.Stress));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatHintStrengthTest, "CotS.Heartbeat.HintStrengthRisesWithStress", HeartbeatTestFlags)
bool FCotSHeartbeatHintStrengthTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	TestEqual(TEXT("A calm heart sends the calm strength"), CotSHeartbeat::HintStrength(Settings, 0.f), Settings.CalmHintStrength);
	TestEqual(TEXT("A heart at full stress sends the loud strength"), CotSHeartbeat::HintStrength(Settings, 1.f), Settings.LoudHintStrength);
	TestTrue(TEXT("More stress is a stronger hint"), CotSHeartbeat::HintStrength(Settings, 0.8f) > CotSHeartbeat::HintStrength(Settings, 0.2f));

	// The director drops a hint below its MinHintStrength, which is 0.1 by default.
	TestTrue(TEXT("The hint of a calm heart stays below the drop level of the director"), Settings.CalmHintStrength < 0.1f);
	TestTrue(TEXT("The hint of a loud heart stays above the drop level of the director"), Settings.LoudHintStrength > 0.1f);

	// A value out of range must not break the strength.
	TestEqual(TEXT("Stress below 0 gives the calm strength"), CotSHeartbeat::HintStrength(Settings, -5.f), Settings.CalmHintStrength);
	TestEqual(TEXT("Stress above 1 gives the loud strength"), CotSHeartbeat::HintStrength(Settings, 5.f), Settings.LoudHintStrength);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatPulseTest, "CotS.Heartbeat.BeatShowsMoreOfTheWorld", HeartbeatTestFlags)
bool FCotSHeartbeatPulseTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState State{};
	State.TimeToNextBeat = 0.f;

	const int32 Beats{CotSHeartbeat::Step(Settings, CalmSenses(), State, TestStep)};
	TestEqual(TEXT("The beat comes when the time runs out"), Beats, 1);
	TestTrue(TEXT("The beat shows a part of the world"), State.Pulse > 0.f);

	// The pulse fades away before the next beat.
	CountBeats(Settings, CalmSenses(), State, Settings.PulseFallSeconds + TestStep);
	const float Faded{State.Pulse};

	FCotSHeartbeatState Fresh{};
	Fresh.TimeToNextBeat = 0.f;
	CotSHeartbeat::Step(Settings, CalmSenses(), Fresh, TestStep);
	TestTrue(TEXT("The pulse fades after the beat"), Faded < Fresh.Pulse);

	// A monster close by makes the world more clear.
	FCotSHeartbeatState Hunted{};
	Settle(Settings, MonsterSenses(100.f), Hunted);
	TestTrue(TEXT("A close monster shows more of the world"), CotSHeartbeat::PulseStrength(Settings, Hunted.Stress) > CotSHeartbeat::PulseStrength(Settings, 0.f));
	TestTrue(TEXT("A close monster opens a wider paint radius"), CotSHeartbeat::RevealRadius(Settings, Hunted.Stress) > CotSHeartbeat::RevealRadius(Settings, 0.f));
	TestEqual(TEXT("A calm heart opens the calm radius"), CotSHeartbeat::RevealRadius(Settings, 0.f), Settings.CalmRevealRadius);
	TestEqual(TEXT("A heart at full stress opens the loud radius"), CotSHeartbeat::RevealRadius(Settings, 1.f), Settings.LoudRevealRadius);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatLongFrameTest, "CotS.Heartbeat.LongFrameDoesNotBurstBeats", HeartbeatTestFlags)
bool FCotSHeartbeatLongFrameTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState State{};
	const int32 Beats{CotSHeartbeat::Step(Settings, CalmSenses(), State, 10.f)};
	TestTrue(TEXT("A long frame gives no more beats than the limit"), Beats <= Settings.MaxBeatsPerStep);
	TestTrue(TEXT("A long frame still gives a beat"), Beats > 0);
	TestTrue(TEXT("The next beat waits a full time again"), State.TimeToNextBeat > 0.f);

	// A step of zero time or less must change nothing.
	FCotSHeartbeatState Still{};
	Still.TimeToNextBeat = 0.f;
	TestEqual(TEXT("A step of no time gives no beat"), CotSHeartbeat::Step(Settings, CalmSenses(), Still, 0.f), 0);
	TestEqual(TEXT("A step of negative time gives no beat"), CotSHeartbeat::Step(Settings, CalmSenses(), Still, -1.f), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatAddStressTest, "CotS.Heartbeat.AddStressJumpsAndCalmsDown", HeartbeatTestFlags)
bool FCotSHeartbeatAddStressTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatState State{};
	State.AddStress(0.5f);
	TestEqual(TEXT("The stress jumps at once"), State.Stress, 0.5f);

	State.AddStress(5.f);
	TestEqual(TEXT("The stress stops at 1"), State.Stress, 1.f);

	State.AddStress(-5.f);
	TestEqual(TEXT("The stress stops at 0"), State.Stress, 0.f);

	// The heart calms down again without help.
	State.AddStress(1.f);
	CountBeats(Settings, CalmSenses(), State, 1.f / Settings.StressFallPerSecond + 1.f);
	TestEqual(TEXT("The heart calms down after a scare"), State.Stress, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatTirednessTest, "CotS.Heartbeat.TirednessRaisesTheRate", HeartbeatTestFlags)
bool FCotSHeartbeatTirednessTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatSenses Tired{CalmSenses()};
	Tired.Tiredness = 1.f;

	FCotSHeartbeatState TiredState{};
	Settle(Settings, Tired, TiredState);

	FCotSHeartbeatState RestedState{};
	Settle(Settings, CalmSenses(), RestedState);

	TestTrue(TEXT("An out of breath player has a faster heart"), TiredState.Stress > RestedState.Stress);
	TestNearlyEqual(TEXT("Full tiredness holds the stress at TirednessStress"), TiredState.Stress, Settings.TirednessStress, 0.01f);

	// A tiredness out of range must not break the stress.
	FCotSHeartbeatSenses Broken{CalmSenses()};
	Broken.Tiredness = 10.f;
	TestTrue(TEXT("A tiredness above 1 keeps the stress in range"), CotSHeartbeat::TargetStress(Settings, Broken) <= 1.f);
	Broken.Tiredness = -10.f;
	TestTrue(TEXT("A tiredness below 0 keeps the stress in range"), CotSHeartbeat::TargetStress(Settings, Broken) >= 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatStrongestSenseTest, "CotS.Heartbeat.StrongestSenseSetsTheRate", HeartbeatTestFlags)
bool FCotSHeartbeatStrongestSenseTest::RunTest(const FString& Parameters)
{
	const FCotSHeartbeatSettings Settings{};

	FCotSHeartbeatSenses All{MonsterSenses(100.f)};
	All.bRunning = true;
	All.Tiredness = 1.f;

	TestEqual(TEXT("All senses together stay at full stress"), CotSHeartbeat::TargetStress(Settings, All), 1.f);

	FCotSHeartbeatSenses RunAndTired{CalmSenses()};
	RunAndTired.bRunning = true;
	RunAndTired.Tiredness = 1.f;
	TestEqual(TEXT("The strongest sense sets the stress"), CotSHeartbeat::TargetStress(Settings, RunAndTired), FMath::Max(Settings.RunStress, Settings.TirednessStress));

	TestEqual(TEXT("A player who does nothing has no stress"), CotSHeartbeat::TargetStress(Settings, CalmSenses()), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatRateOrderTest, "CotS.Heartbeat.RateStaysInRange", HeartbeatTestFlags)
bool FCotSHeartbeatRateOrderTest::RunTest(const FString& Parameters)
{
	FCotSHeartbeatSettings Settings{};

	TestEqual(TEXT("No stress gives the calm rate"), CotSHeartbeat::BeatsPerMinute(Settings, 0.f), Settings.CalmBeatsPerMinute);
	TestEqual(TEXT("Full stress gives the fast rate"), CotSHeartbeat::BeatsPerMinute(Settings, 1.f), Settings.FastBeatsPerMinute);
	TestEqual(TEXT("Stress above 1 gives the fast rate"), CotSHeartbeat::BeatsPerMinute(Settings, 9.f), Settings.FastBeatsPerMinute);
	TestEqual(TEXT("Stress below 0 gives the calm rate"), CotSHeartbeat::BeatsPerMinute(Settings, -9.f), Settings.CalmBeatsPerMinute);
	TestNearlyEqual(TEXT("The calm rate gives one beat each second"), CotSHeartbeat::BeatInterval(Settings, 0.f), 1.f, 0.001f);

	// A designer can set a fast rate below the calm rate. The heart must not beat slower at full stress.
	Settings.CalmBeatsPerMinute = 100.f;
	Settings.FastBeatsPerMinute = 40.f;
	TestTrue(TEXT("A fast rate below the calm rate does not slow the heart down"), CotSHeartbeat::BeatInterval(Settings, 1.f) <= CotSHeartbeat::BeatInterval(Settings, 0.f));
	TestTrue(TEXT("The time between two beats stays above zero"), CotSHeartbeat::BeatInterval(Settings, 1.f) > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatRevealThrottleTest, "CotS.Heartbeat.RacingHeartSkipsSomeRevealPulses", HeartbeatTestFlags)
bool FCotSHeartbeatRevealThrottleTest::RunTest(const FString& Parameters)
{
	FCotSHeartbeatSettings Settings{};
	Settings.MinRevealPulseSeconds = 0.6f;

	TestFalse(TEXT("Just after a pulse, another pulse is not allowed yet"), CotSHeartbeat::ShouldRevealPulse(Settings, 0.f));
	TestFalse(TEXT("Partway to the minimum, a pulse is still not allowed"), CotSHeartbeat::ShouldRevealPulse(Settings, 0.3f));
	TestTrue(TEXT("Right at the minimum, a pulse is allowed"), CotSHeartbeat::ShouldRevealPulse(Settings, 0.6f));
	TestTrue(TEXT("Well past the minimum, a pulse is allowed"), CotSHeartbeat::ShouldRevealPulse(Settings, 5.f));

	// A racing heart beats every 0.35s (170 BPM), far faster than the 0.6s throttle. Simulate beats
	// landing every 0.35s and count how many are actually allowed to open a reveal pulse.
	const float RacingInterval{CotSHeartbeat::BeatInterval(Settings, 1.f)};
	TestTrue(TEXT("A racing heart beats faster than the reveal throttle"), RacingInterval < Settings.MinRevealPulseSeconds);

	float TimeSinceLastPulse{Settings.MinRevealPulseSeconds};
	int32 BeatsSeen{};
	int32 PulsesAllowed{};
	for (float Elapsed{}; Elapsed < 6.f; Elapsed += RacingInterval)
	{
		++BeatsSeen;
		TimeSinceLastPulse += RacingInterval;
		if (CotSHeartbeat::ShouldRevealPulse(Settings, TimeSinceLastPulse))
		{
			++PulsesAllowed;
			TimeSinceLastPulse = 0.f;
		}
	}

	TestTrue(TEXT("A racing heart still beats many times"), BeatsSeen > 10);
	TestTrue(TEXT("Not every racing beat opens a reveal pulse"), PulsesAllowed < BeatsSeen);
	TestTrue(TEXT("A racing heart still opens some reveal pulses"), PulsesAllowed > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSHeartbeatRevealRiseSecondsTest, "CotS.Heartbeat.RevealPulseRiseNeverBeatsTheThrottle", HeartbeatTestFlags)
bool FCotSHeartbeatRevealRiseSecondsTest::RunTest(const FString& Parameters)
{
	FCotSHeartbeatSettings Settings{};
	Settings.MinRevealPulseSeconds = 0.6f;

	// A calm heart (1 beat per second) is already slower than the throttle, so the rise matches the beat.
	TestNearlyEqual(TEXT("A calm heart's rise matches its own beat interval"), CotSHeartbeat::RevealPulseRiseSeconds(Settings, 0.f), CotSHeartbeat::BeatInterval(Settings, 0.f), 0.001f);

	// A racing heart beats faster than the throttle, so the rise takes the throttled time instead,
	// matching how far apart its reveal pulses actually land.
	TestNearlyEqual(TEXT("A racing heart's rise matches the throttle, not its fast beat"), CotSHeartbeat::RevealPulseRiseSeconds(Settings, 1.f), Settings.MinRevealPulseSeconds, 0.001f);
	TestTrue(TEXT("The rise is never shorter than the throttle"), CotSHeartbeat::RevealPulseRiseSeconds(Settings, 1.f) >= Settings.MinRevealPulseSeconds);
	return true;
}

#endif
