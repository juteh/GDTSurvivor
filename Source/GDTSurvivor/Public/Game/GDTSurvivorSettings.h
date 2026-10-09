#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GDTSurvivorSettings.generated.h"

class UUpgradeCatalog;
class USoundClass;
class USoundMix;

// Project-wide settings, shown in Project Settings > Game > GDTSurvivor (stored in DefaultGame.ini).
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "GDTSurvivor"))
class GDTSURVIVOR_API UGDTSurvivorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Upgrades")
	TSoftObjectPtr<UUpgradeCatalog> UpgradeCatalog;

	// Base sound mix of every map; UUserSettingsSubsystem sets the player's volumes in it.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> SoundMix;

	// Controlled by the music volume setting.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	// Controlled by the effects volume setting.
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> SfxSoundClass;
};
