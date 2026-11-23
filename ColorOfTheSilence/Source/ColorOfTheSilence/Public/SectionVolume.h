#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SectionVolume.generated.h"

class UBoxComponent;
class UWorld;

UCLASS(Blueprintable, BlueprintType)
class COLOROFTHESILENCE_API ASectionVolume : public AActor
{
	GENERATED_BODY()

public:
	ASectionVolume();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CotS")
	int32 GetSection() const { return Section; }

	bool ContainsPoint(const FVector& WorldPoint) const;

	// Distance above or below the box when the point is inside it in X and Y. A very large number otherwise.
	double GetVerticalGap(const FVector& WorldPoint) const;

	double GetDistanceToBox(const FVector& WorldPoint) const;

	// Sets the section from the volume that holds the actor. Else it uses the nearest volume and logs the distance.
	static void AssignSectionFromVolume(const AActor& Actor, int32& InOutSection);

	static int32 FindSectionAt(const UWorld* World, const FVector& WorldPoint);

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> Box{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CotS", meta = (AllowPrivateAccess = "true"))
	int32 Section{};
};
