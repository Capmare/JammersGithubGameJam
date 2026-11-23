#include "MonsterStateTreeTasks.h"

#include "AIController.h"
#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "HidingSpot.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterCharacter.h"
#include "MonsterFreezeComponent.h"
#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MonsterStateTreeTasks)

EStateTreeRunStatus FCotSGetHintTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};

	UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(Context.GetOwner())};
	if (!Director)
	{
		UE_LOG(LogCotS, Warning, TEXT("Get Director Hint failed. The world has no director."));
		return EStateTreeRunStatus::Failed;
	}

	Director->GetHint(Data.TargetSection, Data.bRetreat, Data.bHasHintLocation, Data.HintLocation, Data.HintStrength);
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSPickHidingSpotTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};

	const APawn* Pawn{Data.AIController ? Data.AIController->GetPawn() : nullptr};
	const UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(Context.GetOwner())};
	if (!Pawn || !Objectives) return EStateTreeRunStatus::Failed;

	AHidingSpot* Best{};
	float BestDistance{TNumericLimits<float>::Max()};
	for (AHidingSpot* Spot : Objectives->GetHidingSpotsInSection(Data.Section))
	{
		if (Spot->GetSecondsSinceChecked() < Data.RecheckSeconds) continue;

		const float Distance{static_cast<float>(FVector::Dist(Pawn->GetActorLocation(), Spot->GetInspectLocation()))};
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Spot;
		}
	}

	if (!Best) return EStateTreeRunStatus::Failed;

	Best->MarkChecked();
	Data.TargetSpot = Best;
	Data.Destination = Best->GetInspectLocation();
	return Data.bKeepRunning ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSInspectSpotTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};
	if (!Data.Spot) return EStateTreeRunStatus::Failed;

	Data.bPlayerFound = false;
	Data.Remaining = Data.Spot->GetInspectSeconds();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCotSInspectSpotTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};
	if (!Data.Spot) return EStateTreeRunStatus::Failed;

	const AMonsterCharacter* Monster{Data.AIController ? Cast<AMonsterCharacter>(Data.AIController->GetPawn()) : nullptr};
	const UMonsterFreezeComponent* Freeze{Monster ? Monster->GetFreezeComponent() : nullptr};
	if (Freeze && Freeze->IsFrozen()) return EStateTreeRunStatus::Running;

	Data.Remaining -= DeltaTime;
	if (Data.Remaining > 0.f) return EStateTreeRunStatus::Running;

	Data.Spot->MarkChecked();
	Data.bPlayerFound = Data.Spot->IsOccupiedByPlayer();
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSCatchPlayerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data{Context.GetInstanceData(*this)};

	const AMonsterCharacter* Monster{Data.AIController ? Cast<AMonsterCharacter>(Data.AIController->GetPawn()) : nullptr};
	const APawn* Player{UGameplayStatics::GetPlayerPawn(Context.GetOwner(), 0)};
	UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(Context.GetOwner())};
	if (!Monster || !Player || !Objectives) return EStateTreeRunStatus::Failed;

	const UMonsterFreezeComponent* Freeze{Monster->GetFreezeComponent()};
	if (Freeze && (Freeze->IsFrozen() || Freeze->IsSeenByPlayer())) return EStateTreeRunStatus::Failed;

	if (FVector::Dist(Monster->GetActorLocation(), Player->GetActorLocation()) > Data.MaxCatchDistance) return EStateTreeRunStatus::Failed;

	Objectives->ReportPlayerCaught();
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSDropHintTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data{Context.GetInstanceData(*this)};

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(Context.GetOwner())}) Director->DropHintsNear(Data.Location);
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSWaitTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};
	Data.Remaining = Data.Seconds;
	return Data.Remaining > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSWaitTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};
	Data.Remaining -= DeltaTime;
	return Data.Remaining > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

namespace
{
	void UpdateDirectorEvaluator(const FCotSDirectorEvaluator& Evaluator, FStateTreeExecutionContext& Context)
	{
		FCotSDirectorEvaluatorInstanceData& Data{Context.GetInstanceData(Evaluator)};
		if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(Context.GetOwner())})
		{
			Director->GetHint(Data.TargetSection, Data.bRetreat, Data.bHasHintLocation, Data.HintLocation, Data.HintStrength);
			Data.bPlayerDetected = Director->IsPlayerDetected();
		}
	}
}

void FCotSDirectorEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	UpdateDirectorEvaluator(*this, Context);
}

void FCotSDirectorEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UpdateDirectorEvaluator(*this, Context);
}

EStateTreeRunStatus FCotSLatchLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};
	Data.LatchedLocation = Data.Location;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCotSChasePlayerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};

	const APawn* Pawn{Data.AIController ? Data.AIController->GetPawn() : nullptr};
	if (!Pawn || !UGameplayStatics::GetPlayerPawn(Context.GetOwner(), 0)) return EStateTreeRunStatus::Failed;

	if (UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(Context.GetOwner())}) Director->NotifyPlayerFound();

	Data.LostTime = 0.f;
	Data.RepathTime = 0.f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCotSChasePlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};

	const AMonsterCharacter* Monster{Data.AIController ? Cast<AMonsterCharacter>(Data.AIController->GetPawn()) : nullptr};
	APawn* Player{UGameplayStatics::GetPlayerPawn(Context.GetOwner(), 0)};
	UCotSObjectiveSubsystem* Objectives{UCotSObjectiveSubsystem::Get(Context.GetOwner())};
	if (!Monster || !Player || !Objectives) return EStateTreeRunStatus::Failed;

	const UMonsterFreezeComponent* Freeze{Monster->GetFreezeComponent()};
	if (Freeze && Freeze->IsFrozen()) return EStateTreeRunStatus::Running;

	const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(Context.GetOwner())};
	Data.LostTime = (Director && Director->IsPlayerDetected()) ? 0.f : Data.LostTime + DeltaTime;
	if (Data.LostTime >= Data.LoseSeconds) return EStateTreeRunStatus::Failed;

	if (FVector::Dist(Monster->GetActorLocation(), Player->GetActorLocation()) <= Data.CatchDistance)
	{
		Objectives->ReportPlayerCaught();
		return EStateTreeRunStatus::Succeeded;
	}

	Data.RepathTime -= DeltaTime;
	if (Data.RepathTime <= 0.f)
	{
		Data.RepathTime = Data.RepathSeconds;
		Data.AIController->MoveToActor(Player, Data.CatchDistance * 0.5f, true, true, true);
	}
	return EStateTreeRunStatus::Running;
}

void FCotSChasePlayerTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data{Context.GetInstanceData(*this)};
	if (Data.AIController) Data.AIController->StopMovement();
}
