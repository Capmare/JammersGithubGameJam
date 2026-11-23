#include "MonsterCharacter.h"

#include "CotSMonsterDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MonsterController.h"
#include "MonsterFreezeComponent.h"

AMonsterCharacter::AMonsterCharacter()
{
	FreezeComponent = CreateDefaultSubobject<UMonsterFreezeComponent>(TEXT("FreezeComponent"));

	AIControllerClass = AMonsterController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AMonsterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->RegisterMonster(this);
}

void AMonsterCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(this)}) Director->UnregisterMonster(this);

	Super::EndPlay(Reason);
}

void AMonsterCharacter::SetDormant(const bool bNewDormant)
{
	AMonsterController* MonsterController{Cast<AMonsterController>(GetController())};

	if (bNewDormant != bDormant)
	{
		bDormant = bNewDormant;
		SetActorHiddenInGame(bDormant);
		SetActorEnableCollision(!bDormant);

		UCharacterMovementComponent* Movement{GetCharacterMovement()};
		if (bDormant)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetDefaultMovementMode();
			if (MonsterController) MonsterController->ResetBehavior();
		}
	}

	// The State Tree can start after the monster registers, so stop it again on every call.
	if (bDormant && MonsterController) MonsterController->StopBehavior();
}
