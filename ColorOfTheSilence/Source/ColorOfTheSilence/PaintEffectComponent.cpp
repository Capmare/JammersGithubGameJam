#include "PaintEffectComponent.h"

#include "CotSPaintReveal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"

namespace
{
// radius limit when the effect is infinite
constexpr float NoLimit = 1000000.0f;
}

DEFINE_LOG_CATEGORY_STATIC(LogPaintEffect, Log, All);

UPaintEffectComponent::UPaintEffectComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UPaintEffectComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Without this, the circle only moves to the player's spot once per beat or Start() call, so it
    // visibly jumps ahead of or behind a moving player instead of following them.
    if (PaintEffect == EPaintEffect::Enabled) UpdateCenter();
}

void UPaintEffectComponent::Start()
{
    if (!PaintCollection)
    {
        UE_LOG(LogPaintEffect, Warning, TEXT("%s: PaintCollection is not set. The effect cannot start."), *GetNameSafe(GetOwner()));
        return;
    }

    UE_LOG(LogPaintEffect, Log, TEXT("%s: Paint effect started."), *GetNameSafe(GetOwner()));

    // A heartbeat pulse may have left ActiveLimit/ActiveSpeed at its own short, small values.
    // The ambient grow must use MaxRadius and Duration, not whatever a pulse last used.
    UseAmbientSpeed();

    if (GetCurrentRadius() <= 0.0f)
    {
        SetScalar(TEXT("EffectMaxRadius"), MaxRadius);
        SetScalar(TEXT("EffectDuration"), Duration);
        SetScalar(TEXT("EffectInfinite"), bInfinite ? 1.0f : 0.0f);

        UpdateCenter();
    }

    SetScalar(TEXT("EffectEnabled"), 1.0f);
    SetDirection(1.0f);
    PaintEffect = EPaintEffect::Enabled;
}

void UPaintEffectComponent::Reverse()
{
    // The world turns off at the ambient speed, even right after a heartbeat pulse.
    UseAmbientSpeed();
    SetDirection(-1.0f);
    PaintEffect = EPaintEffect::Disabled;

}

void UPaintEffectComponent::PulseRadius(float NewRadius, float RiseSeconds)
{
    if (!PaintCollection)
    {
        UE_LOG(LogPaintEffect, Warning, TEXT("%s: PaintCollection is not set. The pulse does nothing."), *GetNameSafe(GetOwner()));
        return;
    }

    const UWorld* world = GetWorld();
    if (!world)
    {
        return;
    }

    // A pulse is always a small, finite radius. It never floods the whole level or the sky.
    const float peak  = FMath::Clamp(NewRadius, 0.0f, MaxRadius);
    // Never start above the peak. A start value bigger than this beat's peak would make the ripple
    // appear fully open at once instead of growing out to it.
    const float start = FMath::Clamp(PulseStartRadius, 0.0f, peak);

    ActiveLimit = peak;
    // The speed covers the real distance (peak - start), so the ripple still lands on its peak
    // exactly at RiseSeconds, the same instant the next beat is due.
    ActiveSpeed = CotSPaintReveal::PulseSpeed(peak - start, RiseSeconds);

    SetScalar(TEXT("EffectMaxRadius"), ActiveLimit);
    SetScalar(TEXT("EffectDuration"), FMath::Max(RiseSeconds, 0.001f));
    SetScalar(TEXT("EffectInfinite"), 0.0f);
    // The shader reads EffectSpeed directly for how fast the radius moves. EffectMaxRadius only
    // clamps how far it can go; it does not, on its own, control the speed.
    SetScalar(TEXT("EffectSpeed"), ActiveSpeed);

    // each beat starts at the player, so the ripple grows out from wherever the player is right now
    UpdateCenter();

    // the pulse starts at PulseStartRadius and grows outward, like a ripple moving away from the player
    RadiusAtChange = start;
    ChangeTime     = world->GetTimeSeconds();
    // the radius grows out to its peak, then holds there until the next beat resets it to the start
    Direction      = 1.0f;

    SetScalar(TEXT("EffectRadiusAtChange"), RadiusAtChange);
    SetScalar(TEXT("EffectChangeTime"), ChangeTime);
    SetScalar(TEXT("EffectDirection"), Direction);
    SetScalar(TEXT("EffectEnabled"), 1.0f);
    PaintEffect = EPaintEffect::Enabled;
}

float UPaintEffectComponent::GetCurrentRadius() const
{
    const UWorld* world = GetWorld();
    if (!world)
    {
        return 0.0f;
    }

    // same formula as the shader
    return CotSPaintReveal::Radius(RadiusAtChange, ChangeTime, Direction, ActiveSpeed, ActiveLimit, world->GetTimeSeconds());
}

void UPaintEffectComponent::UseAmbientSpeed()
{
    ActiveLimit = bInfinite ? NoLimit : MaxRadius;
    ActiveSpeed = CotSPaintReveal::GrowSpeed(MaxRadius, Duration);
}

void UPaintEffectComponent::SetDirection(float newDirection)
{
    const UWorld* world = GetWorld();
    if (!world)
    {
        return;
    }

    // save the radius we have now, the shader continues from here
    // this must happen before we change the direction
    RadiusAtChange = GetCurrentRadius();
    ChangeTime     = world->GetTimeSeconds();
    Direction      = newDirection;

    SetScalar(TEXT("EffectRadiusAtChange"), RadiusAtChange);
    SetScalar(TEXT("EffectChangeTime"), ChangeTime);
    SetScalar(TEXT("EffectDirection"), Direction);
    // The shader reads EffectSpeed directly for how fast the radius moves, not EffectMaxRadius / EffectDuration.
    SetScalar(TEXT("EffectSpeed"), ActiveSpeed);
}

void UPaintEffectComponent::SetScalar(FName name, float value)
{
    UKismetMaterialLibrary::SetScalarParameterValue(this, PaintCollection, name, value);
}

void UPaintEffectComponent::UpdateCenter()
{
    if (!PaintCollection) return;

    const AActor* centerActor = bCenterOnPlayer ? UGameplayStatics::GetPlayerPawn(this, 0) : GetOwner();
    if (!centerActor) return;

    const FVector center = centerActor->GetActorLocation();
    UKismetMaterialLibrary::SetVectorParameterValue(
        this, PaintCollection, TEXT("EffectCenter"), FLinearColor(center.X, center.Y, center.Z));
}