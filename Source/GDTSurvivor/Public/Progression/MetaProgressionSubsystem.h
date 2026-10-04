#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MetaProgressionSubsystem.generated.h"

class UMetaProgressionSaveGame;
class UUpgradeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMetaProgressionChanged);

/**
 * Owns the persistent meta progression (money + permanent upgrades) and saves every change
 * to the "metaprogression" save slot. Lives as long as the game instance, so the upgrade
 * menu and every level read the same data.
 *
 * Price and maximum level come from the UUpgradeDefinition. Permanent upgrades are applied by
 * UShipStatsComponent when the player ship starts, with the same effects as a level-up.
 */
UCLASS()
class GDTSURVIVOR_API UMetaProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 GetMoney() const;

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 GetUpgradeLevel(const UUpgradeDefinition* Upgrade) const;

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	bool CanBuyUpgrade(const UUpgradeDefinition* Upgrade) const;

	// Spends the money and raises the upgrade by one level. Returns false if not affordable or maxed.
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	bool BuyUpgrade(const UUpgradeDefinition* Upgrade);

	// Removes all permanent upgrades and refunds the money spent on them.
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void ResetUpgrades();

	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	void AddMoney(int32 Amount);

	// Broadcast after money or an upgrade level changed.
	UPROPERTY(BlueprintAssignable, Category = "MetaProgression")
	FOnMetaProgressionChanged OnMetaProgressionChanged;

private:
	void Save();

	void HandleChanged();

	UPROPERTY()
	TObjectPtr<UMetaProgressionSaveGame> SaveGame;

	static const FString SaveSlotName;

	// Money of a fresh save, so upgrades can be tested before money can be earned.
	static constexpr int32 StartingMoney = 1000;
};
