#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"
#include "UIButtonDataAsset.generated.h"

UENUM(BlueprintType)
enum class ECotSUIButtonPlatform : uint8
{
	PC,
	PlayStation,
	Xbox
};

USTRUCT(BlueprintType)
struct COLOROFTHESILENCE_API FCotSUIButton
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|UI")
	FSlateBrush Neutral{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|UI")
	FSlateBrush Pressed{};

	// Seconds between two swaps of the neutral and pressed brush
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|UI", meta = (ClampMin = "0.01"))
	float Time{0.5f};

	// The player presses this key to hit the prompt. The asset sets a default for each button
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|UI")
	FKey Key{};
};

UCLASS(Blueprintable)
class COLOROFTHESILENCE_API UUIButtonDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UUIButtonDataAsset();

	// Only the buttons that have a Neutral image, a Pressed image and a Key
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|UI")
	TArray<FCotSUIButton> GetButtons(ECotSUIButtonPlatform Platform) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton Cross{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton Circle{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton Triangle{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton Square{};

	// All four buttons at once, the top one pressed
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton PlayStationUp{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton PlayStationDown{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton PlayStationLeft{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PlayStation")
	FCotSUIButton PlayStationRight{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton A{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton B{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton Y{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton X{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton XboxUp{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton XboxDown{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton XboxLeft{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|Xbox")
	FCotSUIButton XboxRight{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PC")
	FCotSUIButton ArrowUp{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PC")
	FCotSUIButton ArrowDown{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PC")
	FCotSUIButton ArrowLeft{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS|PC")
	FCotSUIButton ArrowRight{};
};
