#include "CotSButtonPromptWidget.h"

#include "Components/Image.h"
#include "TimerManager.h"

void UCotSButtonPromptWidget::SetButton(const FCotSUIButton& NewButton)
{
	Button = NewButton;
	StartSwap();
}

void UCotSButtonPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StartSwap();
}

void UCotSButtonPromptWidget::NativeDestruct()
{
	if (const UWorld* World{GetWorld()}) World->GetTimerManager().ClearTimer(SwapTimer);
	Super::NativeDestruct();
}

void UCotSButtonPromptWidget::StartSwap()
{
	bShowsPressed = false;
	if (Img) Img->SetBrush(Button.Neutral);

	const UWorld* World{GetWorld()};
	if (!World) return;
	World->GetTimerManager().SetTimer(SwapTimer, this, &UCotSButtonPromptWidget::Swap, FMath::Max(Button.Time, 0.01f), true);
}

void UCotSButtonPromptWidget::ShowPressed()
{
	if (const UWorld* World{GetWorld()}) World->GetTimerManager().ClearTimer(SwapTimer);
	bShowsPressed = true;
	if (Img) Img->SetBrush(Button.Pressed);
}

void UCotSButtonPromptWidget::Swap()
{
	bShowsPressed = !bShowsPressed;
	if (Img) Img->SetBrush(bShowsPressed ? Button.Pressed : Button.Neutral);
}
