#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterSpawnPoint.generated.h"

class UArrowComponent;

UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API AMonsterSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawnPoint();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	int32 GetSection() const { return Section; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> Arrow{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	int32 Section{};
};
