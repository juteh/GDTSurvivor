#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetSwitcher.h"
#include "CommonInputTypeEnum.h"
#include "GDTInputMethodSwitcher.generated.h"

/**
 * Widget switcher that follows the player's current input device: child 0 is shown while playing with
 * mouse and keyboard, child 1 while playing with a gamepad. Use it where a key hint is not one icon on
 * both devices (e.g. four CommonActionWidgets W/A/S/D on the keyboard, one stick icon on the gamepad).
 * In the designer, "Active Widget Index" picks the child to preview.
 */
UCLASS(meta = (DisplayName = "Input Method Switcher"))
class GDTSURVIVOR_API UGDTInputMethodSwitcher : public UWidgetSwitcher
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	void HandleInputMethodChanged(ECommonInputType InputType);

	FDelegateHandle InputMethodChangedHandle;
};
