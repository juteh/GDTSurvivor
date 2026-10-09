#include "UI/SettingsMenuWidget.h"

#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "UserSettings/UserSettingsSubsystem.h"

namespace
{
	// Index = EWindowMode::Type.
	const TCHAR* const WindowModeOptions[] = { TEXT("Fullscreen"), TEXT("Windowed Fullscreen"), TEXT("Windowed") };

	// Index 0 = on.
	const TCHAR* const VSyncOptions[] = { TEXT("On"), TEXT("Off") };

	// Index = overall scalability level.
	const TCHAR* const QualityOptions[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Ultra") };

	// Index = EAntiAliasingMode.
	const TCHAR* const AntiAliasingOptions[] = { TEXT("Off"), TEXT("FXAA"), TEXT("TAA"), TEXT("TSR") };

	template <int32 N>
	void SetOptions(UComboBoxString* ComboBox, const TCHAR* const (&Options)[N])
	{
		ComboBox->ClearOptions();
		for (const TCHAR* Option : Options)
		{
			ComboBox->AddOption(Option);
		}
	}
}

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Fill first, then listen: setting the current values must not apply them again.
	FillControls();
	BindControls();
}

void USettingsMenuWidget::NativeDestruct()
{
	UnbindControls();
	if (UUserSettingsSubsystem* Settings = GetUserSettings())
	{
		Settings->SaveSettings();
	}

	Super::NativeDestruct();
}

void USettingsMenuWidget::NativeOnDeactivated()
{
	if (UUserSettingsSubsystem* Settings = GetUserSettings())
	{
		Settings->SaveSettings();
	}

	Super::NativeOnDeactivated();
}

UWidget* USettingsMenuWidget::NativeGetDesiredFocusTarget() const
{
	return Slider_Gamma;
}

UUserSettingsSubsystem* USettingsMenuWidget::GetUserSettings() const
{
	return UUserSettingsSubsystem::GetUserSettings(this);
}

void USettingsMenuWidget::FillControls()
{
	const UUserSettingsSubsystem* Settings = GetUserSettings();
	if (!Settings)
	{
		return;
	}

	Resolutions = Settings->GetSupportedResolutions();
	ComboBoxString_Resolution->ClearOptions();
	for (const FIntPoint& Resolution : Resolutions)
	{
		ComboBoxString_Resolution->AddOption(FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y));
	}
	ComboBoxString_Resolution->SetSelectedIndex(Resolutions.IndexOfByKey(Settings->GetResolution()));

	SetOptions(ComboBoxString_Fullscreen, WindowModeOptions);
	ComboBoxString_Fullscreen->SetSelectedIndex(Settings->GetWindowMode());

	SetOptions(ComboBoxString_vsync, VSyncOptions);
	ComboBoxString_vsync->SetSelectedIndex(Settings->IsVSyncEnabled() ? 0 : 1);

	// A custom quality (-1) selects nothing.
	SetOptions(ComboBoxString_Quality, QualityOptions);
	ComboBoxString_Quality->SetSelectedIndex(Settings->GetQualityLevel());

	SetOptions(ComboBoxString_AntiAliasing, AntiAliasingOptions);
	ComboBoxString_AntiAliasing->SetSelectedIndex(static_cast<int32>(Settings->GetAntiAliasing()));

	Slider_Gamma->SetMinValue(UUserSettingsSubsystem::MinGamma);
	Slider_Gamma->SetMaxValue(UUserSettingsSubsystem::MaxGamma);
	Slider_Gamma->SetValue(Settings->GetGamma());

	Slider_Music->SetMinValue(0.f);
	Slider_Music->SetMaxValue(1.f);
	Slider_Music->SetValue(Settings->GetMusicVolume());

	Slider_Audio->SetMinValue(0.f);
	Slider_Audio->SetMaxValue(1.f);
	Slider_Audio->SetValue(Settings->GetSfxVolume());
}

void USettingsMenuWidget::BindControls()
{
	ComboBoxString_Resolution->OnSelectionChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleResolutionChanged);
	ComboBoxString_Fullscreen->OnSelectionChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleWindowModeChanged);
	ComboBoxString_vsync->OnSelectionChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleVSyncChanged);
	ComboBoxString_Quality->OnSelectionChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleQualityChanged);
	ComboBoxString_AntiAliasing->OnSelectionChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleAntiAliasingChanged);
	Slider_Gamma->OnValueChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleGammaChanged);
	Slider_Music->OnValueChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleMusicVolumeChanged);
	Slider_Audio->OnValueChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleSfxVolumeChanged);
}

void USettingsMenuWidget::UnbindControls()
{
	ComboBoxString_Resolution->OnSelectionChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleResolutionChanged);
	ComboBoxString_Fullscreen->OnSelectionChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleWindowModeChanged);
	ComboBoxString_vsync->OnSelectionChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleVSyncChanged);
	ComboBoxString_Quality->OnSelectionChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleQualityChanged);
	ComboBoxString_AntiAliasing->OnSelectionChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleAntiAliasingChanged);
	Slider_Gamma->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleGammaChanged);
	Slider_Music->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleMusicVolumeChanged);
	Slider_Audio->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleSfxVolumeChanged);
}

void USettingsMenuWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	const int32 Index = ComboBoxString_Resolution->GetSelectedIndex();
	UUserSettingsSubsystem* Settings = GetUserSettings();
	if (Settings && Resolutions.IsValidIndex(Index))
	{
		Settings->SetResolution(Resolutions[Index]);
	}
}

void USettingsMenuWidget::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	const int32 Index = ComboBoxString_Fullscreen->GetSelectedIndex();
	UUserSettingsSubsystem* Settings = GetUserSettings();
	if (Settings && Index >= 0)
	{
		Settings->SetWindowMode(static_cast<EWindowMode::Type>(Index));
	}
}

void USettingsMenuWidget::HandleVSyncChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	const int32 Index = ComboBoxString_vsync->GetSelectedIndex();
	UUserSettingsSubsystem* Settings = GetUserSettings();
	if (Settings && Index >= 0)
	{
		Settings->SetVSyncEnabled(Index == 0);
	}
}

void USettingsMenuWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	const int32 Index = ComboBoxString_Quality->GetSelectedIndex();
	UUserSettingsSubsystem* Settings = GetUserSettings();
	if (Settings && Index >= 0)
	{
		Settings->SetQualityLevel(Index);
	}
}

void USettingsMenuWidget::HandleAntiAliasingChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	const int32 Index = ComboBoxString_AntiAliasing->GetSelectedIndex();
	UUserSettingsSubsystem* Settings = GetUserSettings();
	if (Settings && Index >= 0)
	{
		Settings->SetAntiAliasing(static_cast<EAntiAliasingMode>(Index));
	}
}

void USettingsMenuWidget::HandleGammaChanged(float Value)
{
	if (UUserSettingsSubsystem* Settings = GetUserSettings())
	{
		Settings->SetGamma(Value);
	}
}

void USettingsMenuWidget::HandleMusicVolumeChanged(float Value)
{
	if (UUserSettingsSubsystem* Settings = GetUserSettings())
	{
		Settings->SetMusicVolume(Value);
	}
}

void USettingsMenuWidget::HandleSfxVolumeChanged(float Value)
{
	if (UUserSettingsSubsystem* Settings = GetUserSettings())
	{
		Settings->SetSfxVolume(Value);
	}
}
