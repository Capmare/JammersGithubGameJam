// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Engine/DataAsset.h"
#include "PlayerInputDataAsset.generated.h"

/**
 * REMEMBER TO ADD NEW VALIDATION FOR EACH INPUT
 */
UCLASS(Blueprintable)
class COLOROFTHESILENCE_API UPlayerInputDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	bool CheckAllBindingsValid();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS")
	TObjectPtr<UInputAction> Movement{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS")
	TObjectPtr<UInputAction> Look{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS")
	TObjectPtr<UInputAction> Sprint{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS")
	TObjectPtr<UInputAction> Paint{};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "CotS")
	TObjectPtr<UInputAction> Interact{};
	
};
