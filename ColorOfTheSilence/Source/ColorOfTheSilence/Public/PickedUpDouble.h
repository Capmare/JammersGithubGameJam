#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickedUpDouble.generated.h"

class APlayerCharacter;
class USkeletalMeshComponent;

UCLASS()
class COLOROFTHESILENCE_API APickedUpDouble : public AActor
{
	GENERATED_BODY()

public:
	APickedUpDouble();

	virtual void Tick(float DeltaTime) override;

	void Initialize(const APlayerCharacter* Player);
	void BeginBlend(USkeletalMeshComponent* Parent, FName Socket, const FTransform& Relative, float Seconds);

	USkeletalMeshComponent* GetDoubleMesh() const { return DoubleMesh; }

private:
	void FinishBlend();

	TWeakObjectPtr<USkeletalMeshComponent> BlendParent{};
	FName BlendSocket{};
	FTransform BlendRelative{};
	FTransform BlendStart{};
	float BlendDuration{};
	float BlendElapsed{};

	UPROPERTY(VisibleAnywhere, Category = "CotS")
	TObjectPtr<USceneComponent> Root{};

	UPROPERTY(VisibleAnywhere, Category = "CotS")
	TObjectPtr<USkeletalMeshComponent> DoubleMesh{};
};
