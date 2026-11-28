#include "CotSQTEPromptComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "CotSButtonPromptWidget.h"
#include "MainGameInstance.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UCotSQTEPromptComponent::UCotSQTEPromptComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCotSQTEPromptComponent::StartPrompts(UUIButtonDataAsset* InButtonData, const TSubclassOf<UCotSButtonPromptWidget> InPromptWidgetClass, const ECotSDifficulty FallbackDifficulty)
{
	StopPrompts();
	ButtonData = InButtonData;
	PromptWidgetClass = InPromptWidgetClass;
	if (!ButtonData || !PromptWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("QTE prompts: QTEButtonData %s, QTEPromptWidgetClass %s. Set both on the player."), *GetNameSafe(ButtonData), *GetNameSafe(PromptWidgetClass.Get()));
		return;
	}

	if (!ResolvePlatform())
	{
		UE_LOG(LogTemp, Warning, TEXT("QTE prompts: %s has no button with both images and a key on any platform. No prompt spawns."), *GetNameSafe(ButtonData));
		return;
	}
	if (ButtonData->GetButtons(GetPlatform()).IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("QTE prompts: %s has no usable button for platform %s. The prompts use another platform."), *GetNameSafe(ButtonData), *UEnum::GetValueAsString(GetPlatform()));
	}

	Fails = 0;
	ECotSDifficulty Difficulty{FallbackDifficulty};
	if (const UMainGameInstance* GameInstance{GetWorld()->GetGameInstance<UMainGameInstance>()}; GameInstance && GameInstance->IsDifficultySet()) Difficulty = GameInstance->GetDifficulty();
	Active = Difficulty == ECotSDifficulty::Easy ? Easy : Difficulty == ECotSDifficulty::Hard ? Hard : Normal;

	FailLimit = FMath::RandRange(Active.MinFails, FMath::Max(Active.MinFails, Active.MaxFails));
	UE_LOG(LogTemp, Log, TEXT("QTE prompts: start. Difficulty %s, miss limit %d, %.2f progress per hit"), *UEnum::GetValueAsString(Difficulty), FailLimit, Active.ProgressPerHit);
	SpeedScale = 1.f;
	NextSpawnTime = 0.0;
	SetComponentTickEnabled(true);
}

void UCotSQTEPromptComponent::StopPrompts()
{
	SetComponentTickEnabled(false);
	ClearPromptWidgets();
	ShownPlatform.Reset();
}

void UCotSQTEPromptComponent::ClearPromptWidgets()
{
	for (const FCotSActivePrompt& Prompt : Prompts)
	{
		if (IsValid(Prompt.Widget)) Prompt.Widget->RemoveFromParent();
	}
	Prompts.Reset();
}

void UCotSQTEPromptComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	StopPrompts();
	Super::EndPlay(Reason);
}

void UCotSQTEPromptComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const double Now{GetWorld()->GetTimeSeconds()};

	// A gamepad can connect during the struggle, so the platform is checked on each frame
	const TOptional<ECotSUIButtonPlatform> ActivePlatform{ResolvePlatform()};
	if (ActivePlatform != ShownPlatform)
	{
		ClearPromptWidgets();
		ShownPlatform = ActivePlatform;
		NextSpawnTime = 0.0;
	}
	if (!ActivePlatform) return;

	const TArray<FCotSUIButton> Buttons{ButtonData->GetButtons(*ActivePlatform)};
	if (const APlayerController* PlayerController{GetPlayerController()}) CheckPromptKeys(*PlayerController, Buttons, Now);
	if (!IsComponentTickEnabled()) return;

	int32 TimedOut{};
	for (int32 Index{Prompts.Num() - 1}; Index >= 0; --Index)
	{
		const FCotSActivePrompt& Prompt{Prompts[Index]};
		if (Now < Prompt.EndTime && IsValid(Prompt.Widget)) continue;
		if (!Prompt.bHit) ++TimedOut;
		if (IsValid(Prompt.Widget)) Prompt.Widget->RemoveFromParent();
		Prompts.RemoveAt(Index);
	}
	for (int32 Miss{}; Miss < TimedOut; ++Miss)
	{
		RegisterMiss();
		if (!IsComponentTickEnabled()) return;
	}

	if (Now >= NextSpawnTime && Prompts.Num() < Active.MaxPrompts)
	{
		SpawnPrompt(Buttons);
		NextSpawnTime = Now + Active.SpawnIntervalSeconds * SpeedScale;
	}
}

void UCotSQTEPromptComponent::SpawnPrompt(const TArray<FCotSUIButton>& Buttons)
{
	APlayerController* PlayerController{GetPlayerController()};
	if (!PlayerController || Buttons.IsEmpty()) return;

	UCotSButtonPromptWidget* Prompt{CreateWidget<UCotSButtonPromptWidget>(PlayerController, PromptWidgetClass)};
	if (!Prompt) return;

	const FCotSUIButton& Button{Buttons[FMath::RandHelper(Buttons.Num())]};
	Prompt->SetButton(Button);
	Prompt->AddToViewport(PromptZOrder);
	Prompt->SetAlignmentInViewport(FVector2D{0.5, 0.5});
	Prompt->SetDesiredSizeInViewport(PromptSize);

	const FVector2D Viewport{UWidgetLayoutLibrary::GetViewportSize(this)};
	const FVector2D Position{
		Viewport.X * FMath::FRandRange(ScreenEdgeMargin, 1.f - ScreenEdgeMargin),
		Viewport.Y * FMath::FRandRange(ScreenEdgeMargin, 1.f - ScreenEdgeMargin)};
	Prompt->SetPositionInViewport(Position, true);

	Prompts.Add(FCotSActivePrompt{Prompt, Button.Key, GetWorld()->GetTimeSeconds() + Active.PromptLifetimeSeconds * SpeedScale, false});
}

// Only keys of the shown platform count as a miss, so the stick or the camera never fail a prompt
void UCotSQTEPromptComponent::CheckPromptKeys(const APlayerController& PlayerController, const TArray<FCotSUIButton>& Buttons, const double Now)
{
	FCotSActivePrompt* Next{Prompts.FindByPredicate([](const FCotSActivePrompt& Prompt) { return !Prompt.bHit && IsValid(Prompt.Widget); })};
	if (!Next) return;

	if (PlayerController.WasInputKeyJustPressed(Next->Key))
	{
		Next->bHit = true;
		Next->EndTime = Now + HitShowSeconds;
		Next->Widget->ShowPressed();
		SpeedScale = FMath::Max(Active.MinSpeedScale, SpeedScale * Active.SpeedUpPerHit);
		UE_LOG(LogTemp, Log, TEXT("QTE prompts: hit %s. Speed scale %.2f"), *Next->Key.ToString(), SpeedScale);
		OnPromptHit.Broadcast();
		return;
	}

	const bool bWrongKey{Buttons.ContainsByPredicate([&](const FCotSUIButton& Button)
	{
		return Button.Key != Next->Key && PlayerController.WasInputKeyJustPressed(Button.Key);
	})};
	if (!bWrongKey) return;

	Next->bHit = true;
	Next->EndTime = Now;
	RegisterMiss();
}

void UCotSQTEPromptComponent::RegisterMiss()
{
	++Fails;
	UE_LOG(LogTemp, Log, TEXT("QTE prompts: miss %d of %d"), Fails, FailLimit);
	OnPromptMissed.Broadcast(Fails, FailLimit);
	if (Fails >= FailLimit) OnFailLimitReached.Broadcast();
}

ECotSUIButtonPlatform UCotSQTEPromptComponent::GetPlatform() const
{
	if (!bAutoDetectPlatform) return Platform;

	const APlayerController* PlayerController{GetPlayerController()};
	const UInputDeviceSubsystem* Devices{UInputDeviceSubsystem::Get()};
	if (!PlayerController || !Devices) return Platform;

	const FHardwareDeviceIdentifier Device{Devices->GetMostRecentlyUsedHardwareDevice(PlayerController->GetPlatformUserId())};
	if (Device.PrimaryDeviceType != EHardwareDevicePrimaryType::Gamepad) return ECotSUIButtonPlatform::PC;

	const FString Name{Device.HardwareDeviceIdentifier.ToString()};
	const bool bPlayStation{Name.Contains(TEXT("Dual")) || Name.Contains(TEXT("PlayStation")) || Name.Contains(TEXT("PS4")) || Name.Contains(TEXT("PS5"))};
	return bPlayStation ? ECotSUIButtonPlatform::PlayStation : ECotSUIButtonPlatform::Xbox;
}

TOptional<ECotSUIButtonPlatform> UCotSQTEPromptComponent::ResolvePlatform() const
{
	if (!ButtonData) return {};

	const ECotSUIButtonPlatform Preferred{GetPlatform()};
	if (!ButtonData->GetButtons(Preferred).IsEmpty()) return Preferred;

	for (const ECotSUIButtonPlatform Other : {ECotSUIButtonPlatform::PC, ECotSUIButtonPlatform::PlayStation, ECotSUIButtonPlatform::Xbox})
	{
		if (Other != Preferred && !ButtonData->GetButtons(Other).IsEmpty()) return Other;
	}
	return {};
}

APlayerController* UCotSQTEPromptComponent::GetPlayerController() const
{
	const APawn* Pawn{Cast<APawn>(GetOwner())};
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}
