#include "PaintEffectComponent.h"

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

void UPaintEffectComponent::Start()
{
    if (!PaintCollection)
    {
        UE_LOG(LogPaintEffect, Warning, TEXT("%s: PaintCollection is not set. The effect cannot start."), *GetNameSafe(GetOwner()));
        return;
    }

    UE_LOG(LogPaintEffect, Log, TEXT("%s: Paint effect started."), *GetNameSafe(GetOwner()));

    if (GetCurrentRadius() <= 0.0f)
    {
        SetScalar(TEXT("EffectMaxRadius"), MaxRadius);
        SetScalar(TEXT("EffectDuration"), Duration);
        SetScalar(TEXT("EffectInfinite"), bInfinite ? 1.0f : 0.0f);


        const AActor* centerActor = bCenterOnPlayer ? UGameplayStatics::GetPlayerPawn(this, 0) : GetOwner();
        if (centerActor)
        {
            const FVector center = centerActor->GetActorLocation();
            UKismetMaterialLibrary::SetVectorParameterValue(
                this, PaintCollection, TEXT("EffectCenter"), FLinearColor(center.X, center.Y, center.Z));
        }
    }

    SetScalar(TEXT("EffectEnabled"), 1.0f);
    SetDirection(1.0f);
}

void UPaintEffectComponent::Reverse()
{
    SetDirection(-1.0f);
}

float UPaintEffectComponent::GetCurrentRadius() const
{
    const UWorld* world = GetWorld();
    if (!world)
    {
        return 0.0f;
    }

    // same formula as the shader
    const float speed  = MaxRadius / FMath::Max(Duration, 0.001f);
    const float limit  = bInfinite ? NoLimit : MaxRadius;
    const float passed = world->GetTimeSeconds() - ChangeTime;
    const float raw    = RadiusAtChange + Direction * passed * speed;

    return FMath::Clamp(raw, 0.0f, limit);
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
}

void UPaintEffectComponent::SetScalar(FName name, float value)
{
    UKismetMaterialLibrary::SetScalarParameterValue(this, PaintCollection, name, value);
}