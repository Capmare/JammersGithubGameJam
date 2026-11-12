// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProceduralActor.generated.h"

USTRUCT(BlueprintType)
struct FStaticMeshArray
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UStaticMesh*> Meshes;
};

UCLASS()
class COLOROFTHESILENCE_API ABaseProceduralActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseProceduralActor();

	UPROPERTY(EditDefaultsOnly, Category = "Procedural Generation")
	int32 Room = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Procedural Generation", meta = (Description = "Overrides the default mesh generation, if not set, a random mesh will be generated"))
	TMap<FName, FStaticMeshArray> ThemeMeshOverrides;

	UFUNCTION(BlueprintCallable, Category = "Procedural Generation")
	void GenerateRandomMesh(FName CurrentTheme);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural Generation")
	TArray<UStaticMeshComponent*> MeshComponents;

private:

};
