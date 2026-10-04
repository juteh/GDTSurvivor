#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UpgradeCatalog.generated.h"

class UUpgradeDefinition;

/**
 * List of all upgrades in display order (shop rows, HUD list). Set in
 * Project Settings > Game > GDTSurvivor > Upgrade Catalog.
 */
UCLASS(BlueprintType)
class GDTSURVIVOR_API UUpgradeCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// The catalog from the project settings, or null (with a log error) if none is set.
	static const UUpgradeCatalog* Get();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	TArray<TObjectPtr<UUpgradeDefinition>> Upgrades;

	const UUpgradeDefinition* FindById(FName UpgradeId) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
