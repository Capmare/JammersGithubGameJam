#include "CotSPaintReveal.h"
#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

namespace
{
constexpr EAutomationTestFlags PaintRevealTestFlags{EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSPaintRevealGrowTest, "CotS.PaintReveal.RadiusGrowsAndHoldsAtTheLimit", PaintRevealTestFlags)
bool FCotSPaintRevealGrowTest::RunTest(const FString& Parameters)
{
	// 6000 over 5 seconds is 1200 cm per second.
	const float Speed{CotSPaintReveal::GrowSpeed(6000.f, 5.f)};
	TestNearlyEqual(TEXT("The grow speed matches MaxRadius over Duration"), Speed, 1200.f, 0.01f);

	TestEqual(TEXT("The radius starts at RadiusAtChange"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, 6000.f, 0.f), 0.f);
	TestNearlyEqual(TEXT("Half the duration gives half the radius"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, 6000.f, 2.5f), 3000.f, 0.5f);
	TestEqual(TEXT("The radius holds at the limit once it reaches it"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, 6000.f, 5.f), 6000.f);
	TestEqual(TEXT("The radius does not grow past the limit"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, 6000.f, 60.f), 6000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSPaintRevealShrinkTest, "CotS.PaintReveal.RadiusShrinksAndStopsAtZero", PaintRevealTestFlags)
bool FCotSPaintRevealShrinkTest::RunTest(const FString& Parameters)
{
	// Reverse() uses this direction: leaving the sound world fades the radius back down to 0.
	const float Speed{CotSPaintReveal::GrowSpeed(6000.f, 5.f)};

	TestEqual(TEXT("The radius starts at the peak"), CotSPaintReveal::Radius(6000.f, 0.f, -1.f, Speed, 6000.f, 0.f), 6000.f);
	TestNearlyEqual(TEXT("Half the duration gives half the radius"), CotSPaintReveal::Radius(6000.f, 0.f, -1.f, Speed, 6000.f, 2.5f), 3000.f, 0.5f);
	TestEqual(TEXT("The radius stops at 0"), CotSPaintReveal::Radius(6000.f, 0.f, -1.f, Speed, 6000.f, 5.f), 0.f);
	TestEqual(TEXT("The radius does not fall below 0"), CotSPaintReveal::Radius(6000.f, 0.f, -1.f, Speed, 6000.f, 60.f), 0.f);
	return true;
}

// PulseRadius starts a beat's ripple at 0, at the player, and grows it outward. A ripple that instead
// started at its peak and shrank inward would look like it was closing in on the player, not opening
// out from them.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSPaintRevealPulseGrowsOutwardTest, "CotS.PaintReveal.PulseGrowsOutwardFromThePlayer", PaintRevealTestFlags)
bool FCotSPaintRevealPulseGrowsOutwardTest::RunTest(const FString& Parameters)
{
	constexpr float Peak{1000.f};
	constexpr float RiseSeconds{1.f};
	const float Speed{CotSPaintReveal::PulseSpeed(Peak, RiseSeconds)};

	// RadiusAtChange = 0 and Direction = +1 is what PulseRadius sends: the ripple starts at the player.
	TestEqual(TEXT("The ripple starts at 0, right at the player"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, Peak, 0.f), 0.f);
	TestTrue(TEXT("The ripple has moved outward partway through its rise"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, Peak, RiseSeconds * 0.5f) > 0.f);
	TestEqual(TEXT("The ripple reaches its peak exactly at the rise time"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, Peak, RiseSeconds), Peak);
	TestEqual(TEXT("The ripple does not grow past its peak"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, Speed, Peak, RiseSeconds * 4.f), Peak);
	return true;
}

// This is the bug that made the sound world look like it never switched: a small heartbeat pulse
// used the same speed as the big ambient fade, so it crossed its whole radius almost at once.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSPaintRevealPulseSpeedIsIndependentTest, "CotS.PaintReveal.PulseSpeedDoesNotUseTheAmbientSpeed", PaintRevealTestFlags)
bool FCotSPaintRevealPulseSpeedIsIndependentTest::RunTest(const FString& Parameters)
{
	// A small pulse (1000) with its own rise time (1 second) must take the full second to open,
	// not the ambient speed of a 6000 / 5 second fade, which would open it in well under a second.
	constexpr float Peak{1000.f};
	constexpr float RiseSeconds{1.f};
	const float AmbientSpeed{CotSPaintReveal::GrowSpeed(6000.f, 5.f)};
	const float PulseSpeed{CotSPaintReveal::PulseSpeed(Peak, RiseSeconds)};

	TestTrue(TEXT("The pulse speed is slower than the ambient speed would make it"), PulseSpeed < AmbientSpeed);

	// With the ambient speed, the old bug would have already reached the peak long before the rise time was up.
	const float FinishedByTheOldBug{CotSPaintReveal::Radius(0.f, 0.f, 1.f, AmbientSpeed, Peak, RiseSeconds)};
	TestEqual(TEXT("The old ambient speed would already have finished the pulse"), FinishedByTheOldBug, Peak);

	// With its own speed, the pulse is still partway open, and exactly at its peak at the end.
	const float StillRising{CotSPaintReveal::Radius(0.f, 0.f, 1.f, PulseSpeed, Peak, RiseSeconds * 0.5f)};
	TestTrue(TEXT("The pulse is still rising half way through its own rise time"), StillRising < Peak * 0.6f);

	const float AtPeakAtTheEnd{CotSPaintReveal::Radius(0.f, 0.f, 1.f, PulseSpeed, Peak, RiseSeconds)};
	TestEqual(TEXT("The pulse reaches its peak exactly at its own rise time"), AtPeakAtTheEnd, Peak);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSPaintRevealPulseSpeedScalesWithPeakTest, "CotS.PaintReveal.PulseSpeedScalesWithThePeak", PaintRevealTestFlags)
bool FCotSPaintRevealPulseSpeedScalesWithPeakTest::RunTest(const FString& Parameters)
{
	// A louder pulse must move faster to still cross its own, bigger radius in the same time as a quiet pulse.
	const float QuietSpeed{CotSPaintReveal::PulseSpeed(1000.f, 1.f)};
	const float LoudSpeed{CotSPaintReveal::PulseSpeed(3500.f, 1.f)};
	TestTrue(TEXT("A wider pulse moves faster, so it still opens in the same time"), LoudSpeed > QuietSpeed);

	TestEqual(TEXT("Both pulses reach their peak at exactly their rise time"), CotSPaintReveal::Radius(0.f, 0.f, 1.f, LoudSpeed, 3500.f, 1.f), 3500.f);

	// A rise time of 0 or less must not divide by 0 or give an infinite speed.
	const float Instant{CotSPaintReveal::PulseSpeed(1000.f, 0.f)};
	TestTrue(TEXT("A rise time of 0 gives a finite speed"), FMath::IsFinite(Instant));
	TestTrue(TEXT("A rise time of 0 gives a speed above 0"), Instant > 0.f);

	// A peak at or below 0 must not give a negative speed.
	TestTrue(TEXT("A peak of 0 gives a speed of 0 or more"), CotSPaintReveal::PulseSpeed(0.f, 1.f) >= 0.f);
	TestTrue(TEXT("A negative peak gives a speed of 0 or more"), CotSPaintReveal::PulseSpeed(-500.f, 1.f) >= 0.f);
	return true;
}

#endif
