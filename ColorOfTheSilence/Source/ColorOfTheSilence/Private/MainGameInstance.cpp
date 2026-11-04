


#include "MainGameInstance.h"


UMainGameInstance::UMainGameInstance()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> BP_ThemeData(TEXT("DataTable'/Game/Data/DataTables/DT_ThemeData.DT_ThemeData'"));
	ThemeDataTable = BP_ThemeData.Object;
}

UDataTable* UMainGameInstance::GetThemeDataTable()
{
	return ThemeDataTable;
}

