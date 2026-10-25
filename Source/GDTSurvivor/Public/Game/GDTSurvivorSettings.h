#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GDTSurvivorSettings.generated.h"

class UMineralCatalog;
class UUpgradeCatalog;
class USoundClass;
class USoundMix;
class UUserWidget;

// Project-wide settings, shown in Project Settings > Game > GDTSurvivor (stored in DefaultGame.ini).
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GDTSurvivor"))
class GDTSURVIVOR_API UGDTSurvivorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TSoftObjectPtr<UUpgradeCatalog> UpgradeCatalog;

	UPROPERTY(Config, EditAnywhere, Category = "Minerals")
	TSoftObjectPtr<UMineralCatalog> MineralCatalog;

	// Base sound mix of every map; UUserSettingsSubsystem sets the player's volumes in it.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> SoundMix;

	// Controlled by the music volume setting.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	// Controlled by the effects volume setting.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	// Shown by UGDTLoadingScreenSubsystem while a map loads. Empty = no loading screen.
	// Keep it simple (image, text, material animation): widget animations and Blueprint logic do not run reliably while loading.
	UPROPERTY(Config, EditAnywhere, Category = "Loading")
	TSoftClassPtr<UUserWidget> LoadingScreenWidget;

	// Keeps the loading screen up at least this long, so it does not just flash on fast map changes.
	UPROPERTY(Config, EditAnywhere, Category = "Loading", meta = (ClampMin = "0", Units = "s"))
	float MinimumLoadingScreenTime = 0.5f;
};
