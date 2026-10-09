#include "UserSettings/UserSettingsSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/GDTSurvivorSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

const FString UUserSettingsSubsystem::SaveSlotName = TEXT("usersettings");

void UUserSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveGame = Cast<UUserSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
	if (!SaveGame)
	{
		SaveGame = Cast<UUserSettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(UUserSettingsSaveGame::StaticClass()));
	}

	ApplyAntiAliasing();
	ApplyGamma();

	// The sound mix belongs to the audio device of a world, so the volumes are set again for every map.
	WorldInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UUserSettingsSubsystem::HandleWorldInitialized);
}

void UUserSettingsSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitializedHandle);
	SaveSettings();

	Super::Deinitialize();
}

UUserSettingsSubsystem* UUserSettingsSubsystem::GetUserSettings(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UUserSettingsSubsystem>() : nullptr;
}

// Audio

float UUserSettingsSubsystem::GetMusicVolume() const
{
	return SaveGame->MusicVolume;
}

void UUserSettingsSubsystem::SetMusicVolume(float Volume)
{
	SaveGame->MusicVolume = FMath::Clamp(Volume, 0.f, 1.f);
	bDirty = true;
	ApplyAudio(GetGameInstance()->GetWorld());
}

float UUserSettingsSubsystem::GetSfxVolume() const
{
	return SaveGame->SfxVolume;
}

void UUserSettingsSubsystem::SetSfxVolume(float Volume)
{
	SaveGame->SfxVolume = FMath::Clamp(Volume, 0.f, 1.f);
	bDirty = true;
	ApplyAudio(GetGameInstance()->GetWorld());
}

void UUserSettingsSubsystem::HandleWorldInitialized(const FActorsInitializedParams& Params)
{
	if (Params.World && Params.World->GetGameInstance() == GetGameInstance())
	{
		ApplyAudio(Params.World);
	}
}

void UUserSettingsSubsystem::ApplyAudio(UWorld* World) const
{
	const UGDTSurvivorSettings* ProjectSettings = GetDefault<UGDTSurvivorSettings>();
	USoundMix* SoundMix = ProjectSettings->SoundMix.LoadSynchronous();
	if (!World || !SoundMix)
	{
		return;
	}

	UGameplayStatics::SetBaseSoundMix(World, SoundMix);
	if (USoundClass* MusicClass = ProjectSettings->MusicSoundClass.LoadSynchronous())
	{
		UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, MusicClass, SaveGame->MusicVolume, 1.f, 0.f);
	}
	if (USoundClass* SfxClass = ProjectSettings->SfxSoundClass.LoadSynchronous())
	{
		UGameplayStatics::SetSoundMixClassOverride(World, SoundMix, SfxClass, SaveGame->SfxVolume, 1.f, 0.f);
	}
}

// Display

EAntiAliasingMode UUserSettingsSubsystem::GetAntiAliasing() const
{
	return SaveGame->AntiAliasing;
}

void UUserSettingsSubsystem::SetAntiAliasing(EAntiAliasingMode Mode)
{
	SaveGame->AntiAliasing = Mode;
	bDirty = true;
	ApplyAntiAliasing();
}

void UUserSettingsSubsystem::ApplyAntiAliasing() const
{
	// Values of r.AntiAliasingMethod.
	int32 Method = 4;
	switch (SaveGame->AntiAliasing)
	{
	case EAntiAliasingMode::Off:  Method = 0; break;
	case EAntiAliasingMode::FXAA: Method = 1; break;
	case EAntiAliasingMode::TAA:  Method = 2; break;
	case EAntiAliasingMode::TSR:  Method = 4; break;
	}

	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
	{
		CVar->Set(Method, ECVF_SetByGameSetting);
	}
}

float UUserSettingsSubsystem::GetGamma() const
{
	return SaveGame->Gamma;
}

void UUserSettingsSubsystem::SetGamma(float Gamma)
{
	SaveGame->Gamma = FMath::Clamp(Gamma, MinGamma, MaxGamma);
	bDirty = true;
	ApplyGamma();
}

void UUserSettingsSubsystem::ApplyGamma() const
{
	if (GEngine)
	{
		GEngine->DisplayGamma = SaveGame->Gamma;
	}
}

// Graphics

UGameUserSettings* UUserSettingsSubsystem::GetGameUserSettings()
{
	return GEngine ? GEngine->GetGameUserSettings() : nullptr;
}

void UUserSettingsSubsystem::ApplyGameUserSettings()
{
	if (UGameUserSettings* Settings = GetGameUserSettings())
	{
		// Also writes GameUserSettings.ini.
		Settings->ApplySettings(false);
	}
}

TArray<FIntPoint> UUserSettingsSubsystem::GetSupportedResolutions() const
{
	TArray<FIntPoint> Resolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	if (Resolutions.IsEmpty())
	{
		Resolutions.Add(GetResolution());
	}

	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });
	return Resolutions;
}

FIntPoint UUserSettingsSubsystem::GetResolution() const
{
	const UGameUserSettings* Settings = GetGameUserSettings();
	return Settings ? Settings->GetScreenResolution() : FIntPoint::ZeroValue;
}

void UUserSettingsSubsystem::SetResolution(FIntPoint Resolution)
{
	if (UGameUserSettings* Settings = GetGameUserSettings())
	{
		Settings->SetScreenResolution(Resolution);
		ApplyGameUserSettings();
	}
}

TEnumAsByte<EWindowMode::Type> UUserSettingsSubsystem::GetWindowMode() const
{
	const UGameUserSettings* Settings = GetGameUserSettings();
	return Settings ? Settings->GetFullscreenMode() : EWindowMode::Fullscreen;
}

void UUserSettingsSubsystem::SetWindowMode(TEnumAsByte<EWindowMode::Type> WindowMode)
{
	if (UGameUserSettings* Settings = GetGameUserSettings())
	{
		Settings->SetFullscreenMode(WindowMode);
		ApplyGameUserSettings();
	}
}

bool UUserSettingsSubsystem::IsVSyncEnabled() const
{
	const UGameUserSettings* Settings = GetGameUserSettings();
	return Settings && Settings->IsVSyncEnabled();
}

void UUserSettingsSubsystem::SetVSyncEnabled(bool bEnabled)
{
	if (UGameUserSettings* Settings = GetGameUserSettings())
	{
		Settings->SetVSyncEnabled(bEnabled);
		ApplyGameUserSettings();
	}
}

int32 UUserSettingsSubsystem::GetQualityLevel() const
{
	const UGameUserSettings* Settings = GetGameUserSettings();
	return Settings ? Settings->GetOverallScalabilityLevel() : -1;
}

void UUserSettingsSubsystem::SetQualityLevel(int32 Level)
{
	if (UGameUserSettings* Settings = GetGameUserSettings())
	{
		Settings->SetOverallScalabilityLevel(FMath::Clamp(Level, 0, 3));
		ApplyGameUserSettings();
	}
}

void UUserSettingsSubsystem::SaveSettings()
{
	if (bDirty && SaveGame)
	{
		UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
		bDirty = false;
	}
}
