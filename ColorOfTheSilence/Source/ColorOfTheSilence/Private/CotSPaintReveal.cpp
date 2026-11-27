#include "CotSPaintReveal.h"

float CotSPaintReveal::Radius(const float RadiusAtChange, const float ChangeTime, const float Direction, const float Speed, const float Limit, const float Now)
{
	const float Passed{Now - ChangeTime};
	const float Raw{RadiusAtChange + Direction * Passed * Speed};
	return FMath::Clamp(Raw, 0.0f, Limit);
}

float CotSPaintReveal::PulseSpeed(const float Peak, const float Seconds)
{
	return FMath::Max(Peak, 0.f) / FMath::Max(Seconds, 0.001f);
}

float CotSPaintReveal::GrowSpeed(const float MaxRadius, const float Duration)
{
	return FMath::Max(MaxRadius, 0.f) / FMath::Max(Duration, 0.001f);
}
