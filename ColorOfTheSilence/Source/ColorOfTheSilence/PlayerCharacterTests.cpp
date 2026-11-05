#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "CotSGameMode.h"
#include "PaintEffectComponent.h"
#include "PlayerCharacter.h"
#include "PlayerInputDataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerInputAssetValidTest, "ColorOfTheSilence.Player.InputAssetValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FPlayerInputAssetValidTest::RunTest(const FString& Parameters)
{
	UPlayerInputDataAsset* Asset{NewObject<UPlayerInputDataAsset>()};
	Asset->Movement = NewObject<UInputAction>();
	Asset->Look = NewObject<UInputAction>();
	Asset->Sprint = NewObject<UInputAction>();
	Asset->Paint = NewObject<UInputAction>();

	TestTrue(TEXT("All actions set is valid"), Asset->CheckAllBindingsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerCameraSetupTest, "ColorOfTheSilence.Player.ThirdPersonCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FPlayerCameraSetupTest::RunTest(const FString& Parameters)
{
	const APlayerCharacter* Player{GetDefault<APlayerCharacter>()};
	const USpringArmComponent* Boom{Player->FindComponentByClass<USpringArmComponent>()};
	const UCameraComponent* Camera{Player->FindComponentByClass<UCameraComponent>()};

	TestNotNull(TEXT("Spring arm exists"), Boom);
	TestNotNull(TEXT("Camera exists"), Camera);
	TestNotNull(TEXT("Paint effect exists"), Player->FindComponentByClass<UPaintEffectComponent>());
	if (!Boom || !Camera) return false;

	TestTrue(TEXT("Arm follows controller rotation"), Boom->bUsePawnControlRotation);
	TestTrue(TEXT("Camera is on the right shoulder"), Boom->SocketOffset.Y > 0.0);
	TestTrue(TEXT("Camera attaches to the arm"), Camera->GetAttachParent() == Boom);
	TestTrue(TEXT("Character turns with the camera"), Player->bUseControllerRotationYaw);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCotSGameModeTest, "ColorOfTheSilence.Player.GameModeUsesPlayerCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FCotSGameModeTest::RunTest(const FString& Parameters)
{
	const ACotSGameMode* GameMode{GetDefault<ACotSGameMode>()};
	TestTrue(TEXT("Default pawn is the player character"), GameMode->DefaultPawnClass == APlayerCharacter::StaticClass());
	return true;
}

#endif
