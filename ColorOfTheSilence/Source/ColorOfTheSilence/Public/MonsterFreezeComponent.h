#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MonsterFreezeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCotSFreezeChanged, bool, bFrozen);

UCLASS(ClassGroup = "CotS", meta = (BlueprintSpawnableComponent))
class COLOROFTHESILENCE_API UMonsterFreezeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMonsterFreezeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsFrozen() const { return bFrozen; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	bool IsSeenByPlayer() const { return bSeen; }

	UPROPERTY(BlueprintAssignable, Category = "CotS|Events")
	FOnCotSFreezeChanged OnFreezeChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS")
	bool bFreezeOnlyWhenAware{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (ClampMin = "0"))
	float MaxSeenDistance{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (ClampMin = "0"))
	float ViewAngleMargin{8.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (ClampMin = "0"))
	float UnseenDelay{0.15f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS")
	TEnumAsByte<ECollisionChannel> SightChannel{ECC_Visibility};

private:
	bool ComputeSeen() const;
	void SetFrozen(bool bNewFrozen);

	bool bFrozen{};
	bool bSeen{};
	float UnseenTime{};
	float SavedMaxWalkSpeed{};
	bool bSavedOrientToMovement{};
	bool bSavedDesiredRotation{};
	bool bSavedControllerYaw{};
};
