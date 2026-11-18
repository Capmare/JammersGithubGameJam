// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "BaseDoor.generated.h"

UCLASS()
class COLOROFTHESILENCE_API ABaseDoor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseDoor();

	virtual void PickUp_Implementation(AActor* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	int32 AssociatedRoom = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
