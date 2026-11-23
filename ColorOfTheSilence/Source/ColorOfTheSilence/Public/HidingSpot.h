#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HidingSpot.generated.h"

class USceneComponent;

UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API AHidingSpot : public AActor
{
	GENERATED_BODY()

public:
	AHidingSpot();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	int32 GetSection() const { return Section; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	FVector GetInspectLocation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	float GetSecondsSinceChecked() const;

	UFUNCTION(BlueprintCallable, Category = "CotS")
	void MarkChecked();

	UFUNCTION(BlueprintCallable, Category = "CotS")
	bool IsOccupiedByPlayer() const { return bPlayerInside; }

	UFUNCTION(BlueprintCallable, Category = "CotS")
	void SetPlayerInside(bool bInside) { bPlayerInside = bInside; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	float GetInspectSeconds() const { return InspectSeconds; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SpotRoot{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> InspectPoint{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	int32 Section{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float InspectSeconds{2.f};

private:
	double LastCheckedTime{-1.0};
	bool bPlayerInside{};
};
