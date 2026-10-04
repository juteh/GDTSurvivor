#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GDTSurvivorSettings.generated.h"

class UUpgradeCatalog;

// Project-wide settings, shown in Project Settings > Game > GDTSurvivor (stored in DefaultGame.ini).
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GDTSurvivor"))
class GDTSURVIVOR_API UGDTSurvivorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TSoftObjectPtr<UUpgradeCatalog> UpgradeCatalog;
};
