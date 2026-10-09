#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UserSettings/UserSettingsSaveGame.h"
#include "UserSettingsSubsystem.generated.h"

class UWorld;
struct FActorsInitializedParams;

/**
 * Owns the player's settings and the only place that applies them. Menus read and change settings only
 * through this subsystem.
 * - Volumes, anti-aliasing and gamma: own save slot "usersettings", applied at startup and on every map.
 * - Resolution, window mode, VSync and quality: forwarded to UGameUserSettings, which stores them in
 *   GameUserSettings.ini and applies them at startup itself.
 * Setters apply immediately; SaveSettings writes the slot (the settings menu calls it when it closes).
 * See Docs/Architecture.md, section 4.
 */
UCLASS()
class GDTSURVIVOR_API UUserSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Shortcut for Blueprints and C++ code that only has a world context.
	UFUNCTION(BlueprintPure, Category = "UserSettings", meta = (WorldContext = "WorldContextObject", DisplayName = "Get User Settings"))
	static UUserSettingsSubsystem* GetUserSettings(const UObject* WorldContextObject);

	// Audio (0..1)

	UFUNCTION(BlueprintPure, Category = "UserSettings|Audio")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Audio")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "UserSettings|Audio")
	float GetSfxVolume() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Audio")
	void SetSfxVolume(float Volume);

	// Display

	UFUNCTION(BlueprintPure, Category = "UserSettings|Display")
	EAntiAliasingMode GetAntiAliasing() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Display")
	void SetAntiAliasing(EAntiAliasingMode Mode);

	UFUNCTION(BlueprintPure, Category = "UserSettings|Display")
	float GetGamma() const;

	// Clamped to MinGamma..MaxGamma.
	UFUNCTION(BlueprintCallable, Category = "UserSettings|Display")
	void SetGamma(float Gamma);

	static constexpr float MinGamma = 1.7f;
	static constexpr float MaxGamma = 2.7f;

	// Graphics (stored by UGameUserSettings)

	// Fullscreen resolutions of the current monitor, smallest first.
	UFUNCTION(BlueprintPure, Category = "UserSettings|Graphics")
	TArray<FIntPoint> GetSupportedResolutions() const;

	UFUNCTION(BlueprintPure, Category = "UserSettings|Graphics")
	FIntPoint GetResolution() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Graphics")
	void SetResolution(FIntPoint Resolution);

	UFUNCTION(BlueprintPure, Category = "UserSettings|Graphics")
	TEnumAsByte<EWindowMode::Type> GetWindowMode() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Graphics")
	void SetWindowMode(TEnumAsByte<EWindowMode::Type> WindowMode);

	UFUNCTION(BlueprintPure, Category = "UserSettings|Graphics")
	bool IsVSyncEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Graphics")
	void SetVSyncEnabled(bool bEnabled);

	// 0 = low … 3 = epic, -1 = custom (single scalability groups changed).
	UFUNCTION(BlueprintPure, Category = "UserSettings|Graphics")
	int32 GetQualityLevel() const;

	UFUNCTION(BlueprintCallable, Category = "UserSettings|Graphics")
	void SetQualityLevel(int32 Level);

	// Writes volumes, anti-aliasing and gamma to the save slot if one of them changed.
	UFUNCTION(BlueprintCallable, Category = "UserSettings")
	void SaveSettings();

private:
	static UGameUserSettings* GetGameUserSettings();
	static void ApplyGameUserSettings();

	void HandleWorldInitialized(const FActorsInitializedParams& Params);
	void ApplyAudio(UWorld* World) const;
	void ApplyAntiAliasing() const;
	void ApplyGamma() const;

	UPROPERTY()
	TObjectPtr<UUserSettingsSaveGame> SaveGame;

	bool bDirty = false;

	FDelegateHandle WorldInitializedHandle;

	static const FString SaveSlotName;
};
