#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GDTControlHintRow.generated.h"

class UTextBlock;

/**
 * Parent class of WBP_ControlHintRow: one line of the controls page in the manual, a label on the left and the
 * key hints on the right. The hints sit in two named slots ("Keyboard", "Gamepad") inside a
 * UGDTInputMethodSwitcher, so every row instance fills them with as many CommonActionWidgets as it needs
 * (W/A/S/D on the keyboard, one stick on the gamepad).
 */
UCLASS(Abstract)
class GDTSURVIVOR_API UGDTControlHintRow : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Control Hint")
	void SetLabel(const FText& InLabel);

protected:
	// Also runs in the designer, so the label shows up in the page layout.
	virtual void NativePreConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Control Hint")
	FText Label;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Label;
};
