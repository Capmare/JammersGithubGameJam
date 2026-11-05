// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerInputDataAsset.h"

bool UPlayerInputDataAsset::CheckAllBindingsValid()
{
	return ensure(Movement) && ensure(Look) && ensure(Sprint) && ensure(Paint);
}
