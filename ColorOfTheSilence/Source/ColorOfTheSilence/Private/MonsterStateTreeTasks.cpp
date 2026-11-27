#include "MonsterStateTreeTasks.h"

#include "AIController.h"
#include "CotSMonsterDirector.h"
#include "CotSObjectiveSubsystem.h"
#include "HidingSpot.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterCharacter.h"
#include "MonsterFreezeComponent.h"
#include "../PlayerCharacter.h"
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

	if (!Best)
	{
		UE_LOG(LogCotS, Log, TEXT("ST Pick spot: no spot in section %d"), Data.Section);
		return EStateTreeRunStatus::Failed;
	}
	UE_LOG(LogCotS, Log, TEXT("ST Pick spot: %s in section %d"), *Best->GetName(), Data.Section);

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
	UE_LOG(LogCotS, Log, TEXT("ST Inspect: %s for %.1f s"), *Data.Spot->GetName(), Data.Remaining);
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
	UE_LOG(LogCotS, Log, TEXT("ST Inspect done: player found %d"), Data.bPlayerFound);
	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FCotSCatchPlayerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data{Context.GetInstanceData(*this)};

	AMonsterCharacter* Monster{Data.AIController ? Cast<AMonsterCharacter>(Data.AIController->GetPawn()) : nullptr};
	APlayerCharacter* Player{Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(Context.GetOwner(), 0))};
	if (!Monster || !Player)
	{
		UE_LOG(LogCotS, Log, TEXT("ST Catch: monster %d player %d"), IsValid(Monster), IsValid(Player));
		return EStateTreeRunStatus::Failed;
	}

	const UMonsterFreezeComponent* Freeze{Monster->GetFreezeComponent()};
	if (Freeze && (Freeze->IsFrozen() || Freeze->IsSeenByPlayer()))
	{
		UE_LOG(LogCotS, Log, TEXT("ST Catch: refused, the player sees the monster"));
		return EStateTreeRunStatus::Failed;
	}

	const double CatchDistance{FVector::Dist(Monster->GetActorLocation(), Player->GetActorLocation())};
	if (CatchDistance > Data.MaxCatchDistance)
	{
		UE_LOG(LogCotS, Log, TEXT("ST Catch: refused, distance %.0f > %.0f"), CatchDistance, Data.MaxCatchDistance);
		return EStateTreeRunStatus::Failed;
	}

	return Monster->GrabPlayer(Player) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FCotSDropHintTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data{Context.GetInstanceData(*this)};

	UE_LOG(LogCotS, Log, TEXT("ST Drop hint at %s"), *Data.Location.ToString());
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
			const bool bOldRetreat{Data.bRetreat};
			const bool bOldHint{Data.bHasHintLocation};
			const bool bOldDetected{Data.bPlayerDetected};
			Director->GetHint(Data.TargetSection, Data.bRetreat, Data.bHasHintLocation, Data.HintLocation, Data.HintStrength);
			Data.bPlayerDetected = Director->IsPlayerDetected();
			if (bOldRetreat != Data.bRetreat || bOldHint != Data.bHasHintLocation || bOldDetected != Data.bPlayerDetected)
			{
				UE_LOG(LogCotS, Log, TEXT("ST Director data: retreat %d hint %d (strength %.2f, section %d) detected %d"), Data.bRetreat, Data.bHasHintLocation, Data.HintStrength, Data.TargetSection, Data.bPlayerDetected);
			}
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
	UE_LOG(LogCotS, Log, TEXT("ST Chase: enter"));
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCotSChasePlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data{Context.GetInstanceData(*this)};

	AMonsterCharacter* Monster{Data.AIController ? Cast<AMonsterCharacter>(Data.AIController->GetPawn()) : nullptr};
	APlayerCharacter* Player{Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(Context.GetOwner(), 0))};
	if (!Monster || !Player)
	{
		UE_LOG(LogCotS, Log, TEXT("ST Chase: monster %d player %d"), IsValid(Monster), IsValid(Player));
		return EStateTreeRunStatus::Failed;
	}

	const UMonsterFreezeComponent* Freeze{Monster->GetFreezeComponent()};
	if (Freeze && Freeze->IsFrozen()) return EStateTreeRunStatus::Running;

	const UCotSMonsterDirector* Director{UCotSMonsterDirector::Get(Context.GetOwner())};
	Data.LostTime = (Director && Director->IsPlayerDetected()) ? 0.f : Data.LostTime + DeltaTime;
	if (Data.LostTime >= Data.LoseSeconds)
	{
		UE_LOG(LogCotS, Log, TEXT("ST Chase: lost the player after %.1f s"), Data.LostTime);
		return EStateTreeRunStatus::Failed;
	}

	if (FVector::Dist(Monster->GetActorLocation(), Player->GetActorLocation()) <= Data.CatchDistance && Monster->GrabPlayer(Player))
	{
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
