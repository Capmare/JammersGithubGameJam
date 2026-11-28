#include "UIButtonDataAsset.h"

UUIButtonDataAsset::UUIButtonDataAsset()
{
	Cross.Key = EKeys::Gamepad_FaceButton_Bottom;
	Circle.Key = EKeys::Gamepad_FaceButton_Right;
	Square.Key = EKeys::Gamepad_FaceButton_Left;
	Triangle.Key = EKeys::Gamepad_FaceButton_Top;

	A.Key = EKeys::Gamepad_FaceButton_Bottom;
	B.Key = EKeys::Gamepad_FaceButton_Right;
	X.Key = EKeys::Gamepad_FaceButton_Left;
	Y.Key = EKeys::Gamepad_FaceButton_Top;

	ArrowUp.Key = EKeys::Up;
	ArrowDown.Key = EKeys::Down;
	ArrowLeft.Key = EKeys::Left;
	ArrowRight.Key = EKeys::Right;
}

TArray<FCotSUIButton> UUIButtonDataAsset::GetButtons(const ECotSUIButtonPlatform Platform) const
{
	TArray<const FCotSUIButton*> All{};
	switch (Platform)
	{
	case ECotSUIButtonPlatform::PlayStation:
		All = {&Cross, &Circle, &Triangle, &Square};
		break;
	case ECotSUIButtonPlatform::Xbox:
		All = {&A, &B, &Y, &X};
		break;
	case ECotSUIButtonPlatform::PC:
		All = {&ArrowUp, &ArrowDown, &ArrowLeft, &ArrowRight};
		break;
	}

	TArray<FCotSUIButton> Result{};
	for (const FCotSUIButton* Button : All)
	{
		if (IsValid(Button->Neutral.GetResourceObject()) && IsValid(Button->Pressed.GetResourceObject()) && Button->Key.IsValid()) Result.Add(*Button);
	}
	return Result;
}
