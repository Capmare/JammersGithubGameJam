#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIButtonDataAsset.h"
#include "CotSButtonPromptWidget.generated.h"

class UImage;

// Swaps the image between the neutral and pressed brush of one button
UCLASS(Abstract, Blueprintable)
class COLOROFTHESILENCE_API UCotSButtonPromptWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "CotS|UI")
	void SetButton(const FCotSUIButton& NewButton);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS|UI")
	const FCotSUIButton& GetButton() const { return Button; }

	// Stops the swap and keeps the pressed image
	UFUNCTION(BlueprintCallable, Category = "CotS|UI")
	void ShowPressed();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "CotS|UI")
	TObjectPtr<UImage> Img{};

private:
	void StartSwap();
	void Swap();

	UPROPERTY()
	FCotSUIButton Button{};

	FTimerHandle SwapTimer{};
	bool bShowsPressed{};
};
