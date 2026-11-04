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

void UPaintEffectComponent::Start()
{

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
    const float passed = world->GetTimeSeconds() - m_ChangeTime;
    const float raw    = m_RadiusAtChange + m_Direction * passed * speed;

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
    m_RadiusAtChange = GetCurrentRadius();
    m_ChangeTime     = world->GetTimeSeconds();
    m_Direction      = newDirection;

    SetScalar(TEXT("EffectRadiusAtChange"), m_RadiusAtChange);
    SetScalar(TEXT("EffectChangeTime"), m_ChangeTime);
    SetScalar(TEXT("EffectDirection"), m_Direction);
}

void UPaintEffectComponent::SetScalar(FName name, float value)
{
    UKismetMaterialLibrary::SetScalarParameterValue(this, PaintCollection, name, value);
}