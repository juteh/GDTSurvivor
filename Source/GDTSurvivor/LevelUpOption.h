// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LevelUpOption.generated.h"

UENUM(BlueprintType)
enum class EUpgradeType : uint8
{
	FireRate,
	Damage,
	Health,
	Shield,
	ShieldRegen,
	PickupRange
};

USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FLevelUpOption : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp")
	EUpgradeType Type = EUpgradeType::FireRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp")
	FText Description;

	// Percentage (e.g. 10.0 = +10%) for percent-based upgrades, flat amount for flat upgrades (Damage, Health, Shield).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp")
	float ValuePerStack = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelUp")
	int32 MaxStacks = 10;
};
