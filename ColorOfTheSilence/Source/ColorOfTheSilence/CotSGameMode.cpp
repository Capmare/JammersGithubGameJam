// Fill out your copyright notice in the Description page of Project Settings.


#include "CotSGameMode.h"

#include "PlayerCharacter.h"

ACotSGameMode::ACotSGameMode()
{
	DefaultPawnClass = APlayerCharacter::StaticClass();
}
