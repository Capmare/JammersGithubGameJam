#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CotSHeartbeat.h"
#include "CotSHeartbeatComponent.generated.h"

class USoundBase;
class UPaintEffectComponent;

// Fires one time for each beat. Strength is 0 to 1 and says how much of the world the beat shows.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCotSHeartbeatSignature, float, Strength, float, Stress);

/**
 * The heart of the player.
 *
 * The heart beats faster when the player runs, when the player is out of breath, and when a monster comes close.
 * It calms down over time when the player stops and the monster leaves.
 * Each beat sends a hint to UCotSMonsterDirector, so a fast heart sends more hints and pulls the monster in.
 * Each beat also shows more of the world. Use OnHeartbeat or GetPulse for that.
 * The heart only beats in the sound world. Set bOnlyInSoundWorld to false to let it beat in both worlds.
 */
UCLASS(ClassGroup = (CotS), meta = (BlueprintSpawnableComponent))
class COLOROFTHESILENCE_API UCotSHeartbeatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCotSHeartbeatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "CotS|Heartbeat")
	FCotSHeartbeatSignature OnHeartbeat;

	// 0 is calm. 1 is full stress.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	float GetStress() const { return State.Stress; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	float GetBeatsPerMinute() const { return CotSHeartbeat::BeatsPerMinute(Settings, State.Stress); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	float GetBeatInterval() const { return CotSHeartbeat::BeatInterval(Settings, State.Stress); }

	// Highest right after a beat. It fades to 0 before the next beat. Use it to show more of the world.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	float GetPulse() const { return State.Pulse; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	bool IsBeating() const { return bBeating; }

	// Raises the stress at once. Use it for a scare. The heart calms down again by itself.
	UFUNCTION(BlueprintCallable, Category = "CotS|Heartbeat")
	void AddStress(float Amount) { State.AddStress(Amount); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat", meta = (ShowOnlyInnerProperties))
	FCotSHeartbeatSettings Settings{};

	// Plays one time for each beat. Use a single beat sound, not a loop.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat")
	TObjectPtr<USoundBase> BeatSound{};

	// Each beat sends a hint to the monster director.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Hint")
	bool bSendHints{true};

	// The name that the director shows for the hints of this heart.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Hint")
	FName HintSource{TEXT("Heartbeat")};

	// The heart only beats while the player is in the sound world.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat")
	bool bOnlyInSoundWorld{true};

	// Each beat opens the paint radius of the player, so the player sees more of the world.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS|Heartbeat|Reveal")
	bool bPulseReveal{true};

	// Paint radius, in centimeters, that the next beat opens.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	float GetRevealRadius() const { return CotSHeartbeat::RevealRadius(Settings, State.Stress); }

	// Seconds since the last reveal pulse actually opened the paint. A racing heart still sends a
	// hint on every beat, but Settings.MinRevealPulseSeconds skips the reveal pulse on some beats.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|Heartbeat")
	float GetTimeSinceLastRevealPulse() const { return TimeSinceLastRevealPulse; }

protected:
	virtual void BeginPlay() override;

	// Reads the player and the director.
	FCotSHeartbeatSenses ReadSenses() const;

	bool IsInSoundWorld() const;

	void Beat();

private:
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "CotS|Heartbeat", meta = (AllowPrivateAccess = "true"))
	FCotSHeartbeatState State{};

	UPROPERTY(Transient)
	TObjectPtr<UPaintEffectComponent> Paint{};

	bool bBeating{};

	// Counts up every tick, in real seconds. Beat() resets it to 0 each time a reveal pulse fires.
	float TimeSinceLastRevealPulse{};
};
