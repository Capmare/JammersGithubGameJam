// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseProceduralActor.h"
#include "DataTableUtility.h"

// Sets default values
ABaseProceduralActor::ABaseProceduralActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ABaseProceduralActor::GenerateRandomMesh(FName CurrentTheme)
{
	MeshComponents.Empty();
	if (ThemeMeshOverrides.Contains(CurrentTheme))
	{
		TArray<UStaticMesh*> Meshes = ThemeMeshOverrides[CurrentTheme].Meshes;
		for (UStaticMesh* Mesh : Meshes)
		{
			if (Mesh)
			{
				UStaticMeshComponent* NewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NewMeshComponent"));
				NewMeshComponent->SetStaticMesh(Mesh);
				NewMeshComponent->SetupAttachment(RootComponent);
				MeshComponents.Add(NewMeshComponent);
			}
		}
		return;
	}
	FThemeData ThemeData = UDataTableUtility::GetThemeData(this, CurrentTheme);
	int32 RandomIndex = FMath::RandRange(0, ThemeData.PossibleAssets.Max() - 1);
	UStaticMeshComponent* NewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NewMeshComponent"));
	NewMeshComponent->SetStaticMesh(ThemeData.PossibleAssets[RandomIndex]);
	NewMeshComponent->SetupAttachment(RootComponent);
	MeshComponents.Add(NewMeshComponent);
}

// Called when the game starts or when spawned
void ABaseProceduralActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseProceduralActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

