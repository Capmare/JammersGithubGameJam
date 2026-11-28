#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CotSDifficulty.h"
#include "UIButtonDataAsset.h"
#include "CotSQTEPromptComponent.generated.h"

class UCotSButtonPromptWidget;

USTRUCT()
struct FCotSActivePrompt
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UCotSButtonPromptWidget> Widget{};

	FKey Key{};
	double EndTime{};
	bool bHit{};
};

// Spawns random button prompts on the screen while the monster holds the player
UCLASS(ClassGroup = (CotS), meta = (BlueprintSpawnableComponent))
class COLOROFTHESILENCE_API UCotSQTEPromptComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCotSQTEPromptComponent();

	UFUNCTION(BlueprintCallable, Category = "CotS|QTE")
	void StartPrompts(UUIButtonDataAsset* InButtonData, TSubclassOf<UCotSButtonPromptWidget> InPromptWidgetClass, ECotSDifficulty FallbackDifficulty);

	UFUNCTION(BlueprintCallable, Category = "CotS|QTE")
	void StopPrompts();

	// Fires once for each prompt the player hits. The player must hit the prompts in the order they appear
	FSimpleMulticastDelegate OnPromptHit;

	// Fires for a wrong key or a prompt that runs out of time. Gives the misses so far and the limit
	TMulticastDelegate<void(int32, int32)> OnPromptMissed;

	FSimpleMulticastDelegate OnFailLimitReached;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|QTE")
	int32 GetFails() const { return Fails; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|QTE")
	int32 GetFailLimit() const { return FailLimit; }

	float GetProgressPerHit() const { return Active.ProgressPerHit; }
	float GetDrainPerSecond() const { return Active.DrainPerSecond; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void SpawnPrompt(const TArray<FCotSUIButton>& Buttons);
	void ClearPromptWidgets();
	void CheckPromptKeys(const APlayerController& PlayerController, const TArray<FCotSUIButton>& Buttons, double Now);
	void RegisterMiss();
	ECotSUIButtonPlatform GetPlatform() const;
	// Falls back to the first platform that has buttons when the current one has none
	TOptional<ECotSUIButtonPlatform> ResolvePlatform() const;
	APlayerController* GetPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UUIButtonDataAsset> ButtonData{};

	UPROPERTY(Transient)
	TSubclassOf<UCotSButtonPromptWidget> PromptWidgetClass{};

	// Off: the component uses Platform. On: it uses the last device the player touched
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE")
	bool bAutoDetectPlatform{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE")
	ECotSUIButtonPlatform Platform{ECotSUIButtonPlatform::PC};

	// In screen pixels
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE")
	FVector2D PromptSize{128.0, 128.0};

	// Part of the screen at each edge where no prompt spawns. 0.15 is 15%
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "0.45"), Category = "CotS|QTE")
	float ScreenEdgeMargin{0.15f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE")
	int32 PromptZOrder{20};

	// How long a hit prompt shows its pressed image before it goes away
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ClampMin = "0"), Category = "CotS|QTE")
	float HitShowSeconds{0.15f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE|Difficulty")
	FCotSQTEDifficulty Easy{1.1f, 3.2f, 2, 7, 10, 0.96f, 0.7f, 0.17f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE|Difficulty")
	FCotSQTEDifficulty Normal{0.85f, 2.4f, 3, 5, 8, 0.92f, 0.5f, 0.125f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "CotS|QTE|Difficulty")
	FCotSQTEDifficulty Hard{0.6f, 1.8f, 4, 4, 7, 0.88f, 0.35f, 0.1f, 0.15f};

	// A copy of Easy, Normal or Hard, taken when the struggle starts
	FCotSQTEDifficulty Active{};

	UPROPERTY(Transient)
	TArray<FCotSActivePrompt> Prompts{};

	TOptional<ECotSUIButtonPlatform> ShownPlatform{};

	float SpeedScale{1.f};
	int32 Fails{};
	int32 FailLimit{};

	double NextSpawnTime{};
};
