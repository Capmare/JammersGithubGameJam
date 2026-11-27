#pragma once

#include "CoreMinimal.h"

// Pure math behind UPaintEffectComponent. No world, no actor, so a test can check it without the engine.
namespace CotSPaintReveal
{
	// Radius at time Now. It moved away from RadiusAtChange at Speed, in the way Direction says, since ChangeTime.
	// The shader uses the same formula, so this must stay in step with it.
	COLOROFTHESILENCE_API float Radius(float RadiusAtChange, float ChangeTime, float Direction, float Speed, float Limit, float Now);

	// Speed, in cm per second, so a radius crosses Peak in Seconds (0 to Peak, or Peak to 0).
	// A heartbeat pulse is far smaller than the ambient MaxRadius, so it needs its own speed.
	// Reusing the ambient speed (MaxRadius / Duration) would cross a small pulse almost at once.
	COLOROFTHESILENCE_API float PulseSpeed(float Peak, float Seconds);

	// Speed, in cm per second, so the ambient radius grows from 0 to MaxRadius in Duration.
	COLOROFTHESILENCE_API float GrowSpeed(float MaxRadius, float Duration);
}
