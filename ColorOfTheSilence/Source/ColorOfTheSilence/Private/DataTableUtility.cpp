


#include "DataTableUtility.h"
#include "GameInstanceInterface.h"
#include "Kismet/GameplayStatics.h"
#include "ThemeData.h"

FThemeData UDataTableUtility::GetThemeData(UObject* WorldContextObject, FName ThemeName)
{
	if (WorldContextObject && WorldContextObject->GetWorld())
	{
		{
			if (UDataTable* Table = Cast<IGameInstanceInterface>(UGameplayStatics::GetGameInstance(WorldContextObject->GetWorld()))->GetThemeDataTable())
			{
				for (FName RowName : Table->GetRowNames())
				{
					FThemeData* Row = Table->FindRow<FThemeData>(RowName, "");
					if (Row && Row->ThemeName == ThemeName)
					{
						return *Row;
					}
				}
			}
		}
	}
	return FThemeData();
}
