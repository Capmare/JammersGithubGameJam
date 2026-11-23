#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MonsterCharacter.generated.h"

class UMonsterFreezeComponent;

UCLASS()
class COLOROFTHESILENCE_API AMonsterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMonsterCharacter();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsDormant() const { return bDormant; }

	// A dormant monster is hidden, has no collision and runs no State Tree.
	void SetDormant(bool bNewDormant);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	UMonsterFreezeComponent* GetFreezeComponent() const { return FreezeComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterFreezeComponent> FreezeComponent{};

	bool bDormant{};
};
