#pragma once

#include "CoreMinimal.h"
#include "CotSHeartbeat.generated.h"

// All values that tune the heart. A designer changes them in the editor or in blueprint.
USTRUCT(BlueprintType)
struct COLOROFTHESILENCE_API FCotSHeartbeatSettings
{
	GENERATED_BODY()

	// Beats per minute when the player is calm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Rate", meta = (ClampMin = "10", ClampMax = "240"))
	float CalmBeatsPerMinute{60.f};

	// Beats per minute at full stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Rate", meta = (ClampMin = "10", ClampMax = "300"))
	float FastBeatsPerMinute{170.f};

	// Stress while the player runs. 1 is full stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0", ClampMax = "1"))
	float RunStress{0.7f};

	// Stress when the player is fully out of breath.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0", ClampMax = "1"))
	float TirednessStress{0.5f};

	// Stress when a monster is at MonsterNearDistance or closer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0", ClampMax = "1"))
	float MonsterStress{1.f};

	// A monster at this distance or closer gives the full MonsterStress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0"))
	float MonsterNearDistance{600.f};

	// A monster at this distance or further gives no stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0"))
	float MonsterFarDistance{2500.f};

	// Stress per second up. 1 means the heart needs 1 second to go from calm to full stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0.01"))
	float StressRisePerSecond{0.8f};

	// Stress per second down. This is how fast the heart calms down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0.01"))
	float StressFallPerSecond{0.25f};

	// The stress target is multiplied by this value while the player calms down.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Stress", meta = (ClampMin = "0", ClampMax = "1"))
	float CalmScale{0.3f};

	// Hint strength of one beat when the player is calm. The director drops a hint below its MinHintStrength.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Hint", meta = (ClampMin = "0"))
	float CalmHintStrength{0.05f};

	// Hint strength of one beat at full stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Hint", meta = (ClampMin = "0"))
	float LoudHintStrength{0.4f};

	// How much of the world one beat shows when the player is calm. 1 is the full amount.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal", meta = (ClampMin = "0", ClampMax = "1"))
	float CalmPulse{0.25f};

	// How much of the world one beat shows at full stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal", meta = (ClampMin = "0", ClampMax = "1"))
	float LoudPulse{1.f};

	// Paint radius, in centimeters, that one beat opens when the player is calm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal", meta = (ClampMin = "0"))
	float CalmRevealRadius{1000.f};

	// Paint radius, in centimeters, that one beat opens at full stress.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal", meta = (ClampMin = "0"))
	float LoudRevealRadius{3500.f};

	// Seconds that one pulse needs to fade to 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal", meta = (ClampMin = "0.01"))
	float PulseFallSeconds{0.35f};

	// Shortest real time between two reveal pulses, no matter how fast the heart beats. A racing
	// heart still sends a hint on every beat, but it skips the reveal pulse on some beats, so the
	// screen does not flash faster than this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal", meta = (ClampMin = "0.05"))
	float MinRevealPulseSeconds{0.6f};

	// Highest number of beats in one step. It stops a burst of beats after a long frame.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Rate", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxBeatsPerStep{3};
};

// What the heart feels. The component reads these values from the player and the director.
USTRUCT(BlueprintType)
struct COLOROFTHESILENCE_API FCotSHeartbeatSenses
{
	GENERATED_BODY()

	// Distance to the nearest monster. A very large value means no monster.
	UPROPERTY(BlueprintReadWrite, Category = "CotS|Heartbeat")
	float MonsterDistance{TNumericLimits<float>::Max()};

	// 0 is fully rested. 1 is fully out of breath.
	UPROPERTY(BlueprintReadWrite, Category = "CotS|Heartbeat")
	float Tiredness{};

	UPROPERTY(BlueprintReadWrite, Category = "CotS|Heartbeat")
	bool bRunning{};

	// True while the player holds the calm key.
	UPROPERTY(BlueprintReadWrite, Category = "CotS|Heartbeat")
	bool bCalming{};
};

// The live state of the heart.
USTRUCT(BlueprintType)
struct COLOROFTHESILENCE_API FCotSHeartbeatState
{
	GENERATED_BODY()

	// 0 is calm. 1 is full stress.
	UPROPERTY(BlueprintReadOnly, Category = "CotS|Heartbeat")
	float Stress{};

	// Seconds until the next beat.
	UPROPERTY(BlueprintReadOnly, Category = "CotS|Heartbeat")
	float TimeToNextBeat{};

	// Highest right after a beat. It fades to 0 between two beats.
	UPROPERTY(BlueprintReadOnly, Category = "CotS|Heartbeat")
	float Pulse{};

	// Raises the stress at once. Use it for a scare. The stress falls back by itself.
	void AddStress(float Amount);
};

// Pure rules of the heart. They use no world and no actor, so a test can check them.
namespace CotSHeartbeat
{
	// 1 when a monster is near. 0 when no monster is in range.
	COLOROFTHESILENCE_API float MonsterStress(float Distance, float NearDistance, float FarDistance);

	// The stress that the senses ask for. The real stress moves to this value over time.
	COLOROFTHESILENCE_API float TargetStress(const FCotSHeartbeatSettings& Settings, const FCotSHeartbeatSenses& Senses);

	COLOROFTHESILENCE_API float BeatsPerMinute(const FCotSHeartbeatSettings& Settings, float Stress);

	// Seconds between two beats.
	COLOROFTHESILENCE_API float BeatInterval(const FCotSHeartbeatSettings& Settings, float Stress);

	// Hint strength of one beat. A fast heart beats more often, so it sends more hints.
	COLOROFTHESILENCE_API float HintStrength(const FCotSHeartbeatSettings& Settings, float Stress);

	// How much of the world one beat shows.
	COLOROFTHESILENCE_API float PulseStrength(const FCotSHeartbeatSettings& Settings, float Stress);

	// Paint radius, in centimeters, that one beat opens. A monster near the player gives a wider radius.
	COLOROFTHESILENCE_API float RevealRadius(const FCotSHeartbeatSettings& Settings, float Stress);

	// True when enough real time has passed since the last reveal pulse. A racing heart beats faster
	// than MinRevealPulseSeconds, so most beats answer false here and only send a hint, with no pulse.
	COLOROFTHESILENCE_API bool ShouldRevealPulse(const FCotSHeartbeatSettings& Settings, float TimeSinceLastRevealPulse);

	// Seconds the next reveal pulse should take to open. It is never shorter than MinRevealPulseSeconds,
	// so a throttled, racing heart still gets one full, visible rise instead of several rushed ones.
	COLOROFTHESILENCE_API float RevealPulseRiseSeconds(const FCotSHeartbeatSettings& Settings, float Stress);

	// Moves the state forward by DeltaTime. It gives back the number of beats in this step.
	COLOROFTHESILENCE_API int32 Step(const FCotSHeartbeatSettings& Settings, const FCotSHeartbeatSenses& Senses, FCotSHeartbeatState& State, float DeltaTime);
}
