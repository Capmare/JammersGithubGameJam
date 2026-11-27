#include "MonsterController.h"

#include "Components/StateTreeAIComponent.h"
#include "CotSObjectiveSubsystem.h"
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
	if (StateTreeComponent->IsRunning())
	{
		UE_LOG(LogCotS, Log, TEXT("Monster AI: stop behavior"));
		StateTreeComponent->StopLogic(TEXT("Dormant"));
	}
}

void AMonsterController::ResetBehavior()
{
	UE_LOG(LogCotS, Log, TEXT("Monster AI: reset behavior (pawn %s, tree %s)"), *GetNameSafe(GetPawn()), *StateTreeAsset.ToString());
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

FString AMonsterController::DescribeBehavior() const
{
	FString States;
#if WITH_GAMEPLAY_DEBUGGER
	for (const FName& Name : StateTreeComponent->GetActiveStateNames()) States += Name.ToString() + TEXT(" > ");
#endif
	return FString::Printf(TEXT("running %d status %d states [%s]"), StateTreeComponent->IsRunning(), static_cast<int32>(StateTreeComponent->GetStateTreeRunStatus()), *States);
}
