#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Ship/ShipStatsComponent.h"
#include "UpgradeDefinition.generated.h"

class UTexture2D;

// What one stack of an upgrade does to one ship stat: (value + Additive) * Multiplier.
USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FUpgradeEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	EShipStat Stat = EShipStat::MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	float Additive = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	float Multiplier = 1.f;
};

/**
 * One upgrade, used both as level-up choice and as permanent upgrade in the shop.
 * Adding a new upgrade = creating one of these assets and listing it in the upgrade catalog.
 */
UCLASS(BlueprintType)
class GDTSURVIVOR_API UUpgradeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Stable id used in save games. Never change it once players have saves.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	FName UpgradeId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TObjectPtr<UTexture2D> Icon;

	// Applied once per stack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade")
	TArray<FUpgradeEffect> Effects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade|Level-Up")
	bool bOfferAsLevelUp = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade|Level-Up", meta = (ClampMin = "1", EditCondition = "bOfferAsLevelUp"))
	int32 MaxLevelUpStacks = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade|Permanent")
	bool bSellAsPermanent = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade|Permanent", meta = (ClampMin = "1", EditCondition = "bSellAsPermanent"))
	int32 MaxPermanentLevel = 10;

	// Price of every permanent level (may become a per-level curve later).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrade|Permanent", meta = (ClampMin = "0", EditCondition = "bSellAsPermanent"))
	int32 PermanentCost = 100;

	// Modifiers for the given number of stacks, tagged with Source.
	void AppendModifiers(int32 Stacks, FName Source, TArray<FShipStatModifier>& OutModifiers) const;
};
