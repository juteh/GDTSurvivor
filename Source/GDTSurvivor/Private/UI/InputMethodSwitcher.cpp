#include "UI/InputMethodSwitcher.h"

#include "CommonInputSubsystem.h"

#define LOCTEXT_NAMESPACE "InputMethodSwitcher"

namespace
{
	const int32 MouseAndKeyboardIndex = 0;
	const int32 GamepadIndex = 1;
}

TSharedRef<SWidget> UInputMethodSwitcher::RebuildWidget()
{
	TSharedRef<SWidget> Widget = Super::RebuildWidget();

	if (!IsDesignTime())
	{
		if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
		{
			InputSubsystem->OnInputMethodChangedNative.Remove(InputMethodChangedHandle);
			InputMethodChangedHandle = InputSubsystem->OnInputMethodChangedNative.AddUObject(
				this, &UInputMethodSwitcher::HandleInputMethodChanged);
			HandleInputMethodChanged(InputSubsystem->GetCurrentInputType());
		}
	}

	return Widget;
}

void UInputMethodSwitcher::ReleaseSlateResources(bool bReleaseChildren)
{
	if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputSubsystem->OnInputMethodChangedNative.Remove(InputMethodChangedHandle);
	}
	InputMethodChangedHandle.Reset();

	Super::ReleaseSlateResources(bReleaseChildren);
}

void UInputMethodSwitcher::HandleInputMethodChanged(ECommonInputType InputType)
{
	const int32 Index = InputType == ECommonInputType::Gamepad ? GamepadIndex : MouseAndKeyboardIndex;
	if (Index < GetNumWidgets())
	{
		SetActiveWidgetIndex(Index);
	}
}

#if WITH_EDITOR
const FText UInputMethodSwitcher::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "GDTSurvivor");
}
#endif

#undef LOCTEXT_NAMESPACE
