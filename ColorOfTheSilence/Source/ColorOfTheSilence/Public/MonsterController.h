#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MonsterController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

UCLASS()
class COLOROFTHESILENCE_API AMonsterController : public AAIController
{
	GENERATED_BODY()

public:
	AMonsterController();

	virtual void PostInitializeComponents() override;

	void ResetBehavior();
	void StopBehavior();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CotS")
	TSoftObjectPtr<UStateTree> StateTreeAsset{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent{};
};
