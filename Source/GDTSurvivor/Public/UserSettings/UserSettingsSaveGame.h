#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UserSettingsSaveGame.generated.h"

// Anti-aliasing method the player can choose. Mapped to r.AntiAliasingMethod by UUserSettingsSubsystem.
UENUM(BlueprintType)
enum class EAntiAliasingMode : uint8
{
	Off,
	FXAA,
	TAA,
	TSR
};

/**
 * The player's settings that the engine does not store itself: volumes, anti-aliasing and gamma.
 * Resolution, window mode, VSync and quality live in UGameUserSettings (GameUserSettings.ini).
 * Read and written only through UUserSettingsSubsystem.
 */
UCLASS()
class GDTSURVIVOR_API UUserSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 0..1
	UPROPERTY(SaveGame)
	float MusicVolume = 1.f;

	// 0..1
	UPROPERTY(SaveGame)
	float SfxVolume = 1.f;

	UPROPERTY(SaveGame)
	EAntiAliasingMode AntiAliasing = EAntiAliasingMode::TSR;

	// Display gamma (engine default 2.2).
	UPROPERTY(SaveGame)
	float Gamma = 2.2f;
};
