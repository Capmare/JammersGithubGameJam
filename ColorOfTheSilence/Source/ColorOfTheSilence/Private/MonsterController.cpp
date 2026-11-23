#include "MonsterController.h"

#include "Components/StateTreeAIComponent.h"
#include "StateTree.h"

AMonsterController::AMonsterController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);
	StateTreeAsset = TSoftObjectPtr<UStateTree>(FSoftObjectPath(TEXT("/Game/AI/ST_Monster.ST_Monster")));
}

void AMonsterController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (UStateTree* Tree{StateTreeAsset.LoadSynchronous()}) StateTreeComponent->SetStateTree(Tree);
}

void AMonsterController::StopBehavior()
{
	StopMovement();
	if (StateTreeComponent->IsRunning()) StateTreeComponent->StopLogic(TEXT("Dormant"));
}

void AMonsterController::ResetBehavior()
{
	StopMovement();
	if (StateTreeComponent->IsRunning()) StateTreeComponent->StopLogic(TEXT("Reset"));
	if (GetPawn()) StateTreeComponent->StartLogic();
}

// The tree needs a possessed pawn. A spawned controller begins play before it possesses, a placed one after.
void AMonsterController::BeginPlay()
{
	Super::BeginPlay();

	if (GetPawn()) ResetBehavior();
}

void AMonsterController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HasActorBegunPlay()) ResetBehavior();
}
