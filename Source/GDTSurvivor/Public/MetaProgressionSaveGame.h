#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MetaProgressionSaveGame.generated.h"

/**
 * Persistent meta progression: the player's money and the permanent upgrades bought with it.
 * Read and written only through UMetaProgressionSubsystem.
 */
UCLASS()
class GDTSURVIVOR_API UMetaProgressionSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	int32 Money = 0;

	// Money spent on permanent upgrades so far, refunded by a reset.
	// Stored separately so a reset stays correct once upgrade costs vary per level.
	UPROPERTY(SaveGame)
	int32 SpentMoney = 0;

	// Bought level per UUpgradeDefinition::UpgradeId. Missing entries mean level 0.
	UPROPERTY(SaveGame)
	TMap<FName, int32> UpgradeLevelsById;
};
