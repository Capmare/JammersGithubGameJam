#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "Tasks/StateTreeAITask.h"
#include "MonsterStateTreeTasks.generated.h"

class AAIController;
class AHidingSpot;

USTRUCT()
struct FCotSGetHintTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Output")
	int32 TargetSection{INDEX_NONE};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bRetreat{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasHintLocation{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector HintLocation{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	float HintStrength{};
};

USTRUCT(meta = (DisplayName = "CotS Get Director Hint", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSGetHintTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSGetHintTaskInstanceData;

	FCotSGetHintTask() { bShouldCallTick = false; bShouldCopyBoundPropertiesOnTick = false; bShouldCopyBoundPropertiesOnExitState = false; }

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FCotSPickHidingSpotTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController{};

	UPROPERTY(EditAnywhere, Category = "Input")
	int32 Section{INDEX_NONE};

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float RecheckSeconds{30.f};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AHidingSpot> TargetSpot{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector Destination{FVector::ZeroVector};

	// Set when the state has child states: a task that ends at once ends the state before a child runs.
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bKeepRunning{};
};

USTRUCT(meta = (DisplayName = "CotS Pick Hiding Spot", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSPickHidingSpotTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSPickHidingSpotTaskInstanceData;

	FCotSPickHidingSpotTask() { bShouldCallTick = false; bShouldCopyBoundPropertiesOnTick = false; bShouldCopyBoundPropertiesOnExitState = false; }

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FCotSInspectSpotTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController{};

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AHidingSpot> Spot{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bPlayerFound{};

	UPROPERTY(Transient)
	float Remaining{};
};

USTRUCT(meta = (DisplayName = "CotS Inspect Hiding Spot", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSInspectSpotTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSInspectSpotTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FCotSCatchPlayerTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController{};

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float MaxCatchDistance{250.f};
};

USTRUCT(meta = (DisplayName = "CotS Catch Player", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSCatchPlayerTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSCatchPlayerTaskInstanceData;

	FCotSCatchPlayerTask() { bShouldCallTick = false; bShouldCopyBoundPropertiesOnTick = false; bShouldCopyBoundPropertiesOnExitState = false; }

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FCotSDropHintTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	FVector Location{FVector::ZeroVector};
};

USTRUCT(meta = (DisplayName = "CotS Drop Hint", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSDropHintTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSDropHintTaskInstanceData;

	FCotSDropHintTask() { bShouldCallTick = false; bShouldCopyBoundPropertiesOnTick = false; bShouldCopyBoundPropertiesOnExitState = false; }

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FCotSWaitTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float Seconds{2.f};

	UPROPERTY(Transient)
	float Remaining{};
};

USTRUCT(meta = (DisplayName = "CotS Wait", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSWaitTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSWaitTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FCotSDirectorEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Output")
	int32 TargetSection{INDEX_NONE};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bRetreat{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bHasHintLocation{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector HintLocation{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	float HintStrength{};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	bool bPlayerDetected{};
};

USTRUCT(meta = (DisplayName = "CotS Director Hint", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSDirectorEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSDirectorEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

USTRUCT()
struct FCotSLatchLocationTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	FVector Location{FVector::ZeroVector};

	UPROPERTY(VisibleAnywhere, Category = "Output")
	FVector LatchedLocation{FVector::ZeroVector};
};

USTRUCT(meta = (DisplayName = "CotS Latch Location", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSLatchLocationTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSLatchLocationTaskInstanceData;

	FCotSLatchLocationTask() { bShouldCallTick = false; bShouldCopyBoundPropertiesOnTick = false; bShouldCopyBoundPropertiesOnExitState = false; }

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FCotSChasePlayerTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController{};

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float CatchDistance{250.f};

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0"))
	float LoseSeconds{8.f};

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.05"))
	float RepathSeconds{0.4f};

	UPROPERTY(Transient)
	float LostTime{};

	UPROPERTY(Transient)
	float RepathTime{};
};

USTRUCT(meta = (DisplayName = "CotS Chase Player", Category = "CotS|Monster"))
struct COLOROFTHESILENCE_API FCotSChasePlayerTask : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCotSChasePlayerTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
