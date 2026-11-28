#pragma once

#include "CoreMinimal.h"
#include "CotSDifficulty.generated.h"

UENUM(BlueprintType)
enum class ECotSDifficulty : uint8
{
	Easy,
	Normal,
	Hard
};

USTRUCT(BlueprintType)
struct COLOROFTHESILENCE_API FCotSQTEDifficulty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "0.05"))
	float SpawnIntervalSeconds{0.6f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "0.1"))
	float PromptLifetimeSeconds{1.8f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "1"))
	int32 MaxPrompts{4};

	// The game picks a random miss limit between MinFails and MaxFails for each grab
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "1"))
	int32 MinFails{4};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "1"))
	int32 MaxFails{7};

	// Each hit multiplies the spawn interval and the lifetime by this value. Lower is a faster speed up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "0.1", ClampMax = "1"))
	float SpeedUpPerHit{0.88f};

	// The fastest speed. 0.35 means 35% of the start interval and lifetime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "0.05", ClampMax = "1"))
	float MinSpeedScale{0.35f};

	// Struggle bar part that one hit fills. 0.1 means 10 hits to escape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ProgressPerHit{0.1f};

	// Struggle bar part that drains each second. 0 turns the drain off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|QTE", meta = (ClampMin = "0"))
	float DrainPerSecond{0.f};
};
