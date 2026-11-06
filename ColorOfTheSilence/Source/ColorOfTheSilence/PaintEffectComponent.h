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
    UFUNCTION(BlueprintCallable, Category = "Paint")
    void Start();

    UFUNCTION(BlueprintCallable, Category = "Paint")
    void Reverse();

    UFUNCTION(BlueprintPure, Category = "Paint")
    float GetCurrentRadius() const;
    
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

private:
    void SetDirection(float newDirection);
    void SetScalar(FName name, float value);

    UPROPERTY(VisibleAnywhere)
    EPaintEffect PaintEffect = EPaintEffect::Disabled;
    
    float RadiusAtChange = 0.0f;
    float ChangeTime     = 0.0f;
    float Direction      = -1.0f;   // +1 = grows, -1 = shrinks
};