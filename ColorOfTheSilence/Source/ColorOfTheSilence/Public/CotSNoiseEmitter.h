#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CotSNoiseEmitter.generated.h"

class USphereComponent;

UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API ACotSNoiseEmitter : public AActor
{
	GENERATED_BODY()

public:
	ACotSNoiseEmitter();

	UFUNCTION(BlueprintCallable, Category = "CotS|Noise")
	void EmitNoise();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void EmitAndSchedule();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> Trigger{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise", meta = (ClampMin = "0"))
	float Loudness{2.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise")
	FName Source{TEXT("Emitter")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise|Timer")
	bool bEmitOnTimer{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise|Timer", meta = (ClampMin = "0", EditCondition = "bEmitOnTimer"))
	float StartDelay{2.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise|Timer", meta = (ClampMin = "0.1", EditCondition = "bEmitOnTimer"))
	float Interval{8.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise|Timer", meta = (ClampMin = "0", EditCondition = "bEmitOnTimer"))
	float IntervalJitter{2.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise|Trigger")
	bool bEmitOnPlayerEnter{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Noise|Trigger", meta = (EditCondition = "bEmitOnPlayerEnter"))
	bool bTriggerOnce{true};

private:
	FTimerHandle TimerHandle;
	bool bTriggered{};
};
