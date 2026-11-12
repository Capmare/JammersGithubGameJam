


#include "MainGameInstance.h"
#include "ThemeData.h"
#include "BaseProceduralActor.h"
#include "Kismet/GameplayStatics.h"


UMainGameInstance::UMainGameInstance()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> BP_ThemeData(TEXT("DataTable'/Game/Data/DataTables/DT_ThemeData.DT_ThemeData'"));
	ThemeDataTable = BP_ThemeData.Object;
}

UDataTable* UMainGameInstance::GetThemeDataTable()
{
	return ThemeDataTable;
}

void UMainGameInstance::RandomizeTheme(int32 Room)
{
	TArray<FName> RowNames = ThemeDataTable->GetRowNames();
	int32 RandomIndex = FMath::RandRange(0, RowNames.Num() - 1);
	FName RandomRowName = RowNames[RandomIndex];
	FThemeData* RandomRowData = ThemeDataTable->FindRow<FThemeData>(RandomRowName, TEXT(""));
	if (RandomRowData)
	{
		TArray<AActor*> ProceduralActors; 
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseProceduralActor::StaticClass(), ProceduralActors);
		for (AActor* Actor : ProceduralActors)
		{
			ABaseProceduralActor* ProceduralActor = Cast<ABaseProceduralActor>(Actor);
			if (ProceduralActor && ProceduralActor->Room == Room)
			{
				ProceduralActor->GenerateRandomMesh(RandomRowData->ThemeName);
			}
		}
	}
}

