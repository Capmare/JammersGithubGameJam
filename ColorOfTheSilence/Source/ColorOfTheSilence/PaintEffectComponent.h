#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "PaintEffectComponent.generated.h"

class UMaterialParameterCollection;

UENUM(BlueprintType)
enum class EPaintEffect : uint8
{
    Enabled,
    Disabled
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UPaintEffectComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPaintEffectComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Paint")
    void Start();

    UFUNCTION(BlueprintCallable, Category = "Paint")
    void Reverse();

    // The radius starts at PulseStartRadius, at the player, and grows out to NewRadius over RiseSeconds,
    // like a ripple moving away from them. The heartbeat calls this on each beat, so each beat opens a
    // ripple that shows more of the world. The paint turns on if it was off.
    // RiseSeconds does not use MaxRadius or Duration, because a pulse is usually far smaller than
    // MaxRadius and needs its own, much shorter time to cross it before the next beat.
    UFUNCTION(BlueprintCallable, Category = "Paint")
    void PulseRadius(float NewRadius, float RiseSeconds = 0.35f);

    UFUNCTION(BlueprintPure, Category = "Paint")
    float GetCurrentRadius() const;

    // Highest radius that the paint can reach.
    UFUNCTION(BlueprintPure, Category = "Paint")
    float GetMaxRadius() const { return MaxRadius; }
    
    UFUNCTION(BlueprintCallable, Category = "Paint")
    EPaintEffect GetPaintEffect() const { return PaintEffect; }

protected:
    UPROPERTY(EditAnywhere, Category = "Paint")
    TObjectPtr<UMaterialParameterCollection> PaintCollection;

    UPROPERTY(EditAnywhere, Category = "Paint", meta = (ClampMin = "1.0"))
    float MaxRadius = 6000.0f;

    UPROPERTY(EditAnywhere, Category = "Paint", meta = (ClampMin = "0.01"))
    float Duration = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Paint")
    bool bInfinite = false;
    
    UPROPERTY(EditAnywhere, Category = "Paint")
    bool bCenterOnPlayer = true;

    // Where a heartbeat pulse starts, instead of right at the player. The material's soft edge
    // (FeatherWidth) is 400 cm, so a floor well above that keeps a beat from collapsing down into
    // the "almost nothing revealed" zone, where the shader shows the plain, unpainted world instead
    // of a small painted circle. It never grows past the pulse's own peak, even if that peak is
    // smaller than this value.
    UPROPERTY(EditAnywhere, Category = "Paint", meta = (ClampMin = "0"))
    float PulseStartRadius = 600.0f;

private:
    void SetDirection(float newDirection);
    void SetScalar(FName name, float value);

    // Moves EffectCenter to where centerActor is now. Start, Reverse and PulseRadius call this once,
    // and TickComponent calls it every frame while the effect is enabled, so the revealed circle follows
    // the player smoothly instead of jumping to a new spot only on the next beat.
    void UpdateCenter();

    // Moves ActiveLimit and ActiveSpeed back to the ambient MaxRadius / Duration. Start and Reverse call
    // this, so they always move at the ambient speed, even right after a heartbeat pulse.
    void UseAmbientSpeed();

    UPROPERTY(VisibleAnywhere)
    EPaintEffect PaintEffect = EPaintEffect::Disabled;

    float RadiusAtChange = 0.0f;
    float ChangeTime     = 0.0f;
    float Direction      = -1.0f;   // +1 = grows, -1 = shrinks

    // The limit and the speed that GetCurrentRadius uses right now. Start, Reverse and PulseRadius each
    // set their own values here, so a short pulse and the long ambient fade never share one speed.
    float ActiveLimit = 0.0f;
    float ActiveSpeed = 0.0f;
};