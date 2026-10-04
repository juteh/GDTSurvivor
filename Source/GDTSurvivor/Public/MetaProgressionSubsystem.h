#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GDTSurvivor/LevelUpOption.h"
#include "MetaProgressionSubsystem.generated.h"

class UMetaProgressionSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMetaProgressionChanged);

/**
 * Owns the persistent meta progression (money + permanent upgrades) and saves every change
 * to the "metaprogression" save slot. Lives as long as the game instance, so the upgrade
 * menu and every level read the same data.
 *
 * Permanent upgrades are applied by UShipStatsComponent when the player ship starts,
 * with the same per-stack effect as a level-up.
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
	int32 GetUpgradeLevel(EUpgradeType Type) const;

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 GetMaxUpgradeLevel(EUpgradeType Type) const;

	// Price of the next level of this upgrade.
	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	int32 GetUpgradeCost(EUpgradeType Type) const;

	UFUNCTION(BlueprintPure, Category = "MetaProgression")
	bool CanBuyUpgrade(EUpgradeType Type) const;

	// Spends the money and raises the upgrade by one level. Returns false if not affordable or maxed.
	UFUNCTION(BlueprintCallable, Category = "MetaProgression")
	bool BuyUpgrade(EUpgradeType Type);

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

	static constexpr int32 UpgradeCost = 100;

	static constexpr int32 MaxUpgradeLevel = 10;
};
