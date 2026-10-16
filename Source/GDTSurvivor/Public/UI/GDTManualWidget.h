#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Engine/DataTable.h"
#include "GDTManualWidget.generated.h"

class UCommonButtonBase;
class UTextBlock;
class UWidgetSwitcher;

/**
 * Parent class of WBP_Manual: a wizard that pages through the game manual (controls, collecting minerals,
 * delivering minerals, intensity …). Every child of PageSwitcher is one page; the designer adds, removes and
 * fills pages in the widget designer only, this class just flips through them. Key hints on a page are
 * CommonActionWidgets, so they show the keyboard or gamepad icon by themselves.
 * Pushed onto WBP_MainMenuStack's MenuStack, closes with DeactivateWidget (button, Escape or gamepad back).
 */
UCLASS(Abstract)
class GDTSURVIVOR_API UGDTManualWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UGDTManualWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Manual")
	void ShowPage(int32 PageIndex);

	UFUNCTION(BlueprintCallable, Category = "Manual")
	void ShowNextPage();

	UFUNCTION(BlueprintCallable, Category = "Manual")
	void ShowPreviousPage();

	UFUNCTION(BlueprintPure, Category = "Manual")
	int32 GetPageIndex() const;

	UFUNCTION(BlueprintPure, Category = "Manual")
	int32 GetPageCount() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	// Called after the page changed, e.g. to play a transition animation.
	UFUNCTION(BlueprintImplementableEvent, Category = "Manual")
	void OnPageChanged(int32 PageIndex);

	// One child per page, in reading order.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> Button_Previous;

	// On the last page this button closes the manual.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> Button_Next;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> Button_Close;

	// Shows "2 / 4".
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PageNumber;

	// Optional shortcuts to flip pages (e.g. TabLeft / TabRight of InputActionDataTable: Q/E, shoulder buttons).
	UPROPERTY(EditDefaultsOnly, Category = "Manual|Input", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle PreviousPageInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Manual|Input", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle NextPageInputAction;

private:
	void HandleNextClicked();
	void UpdatePageControls();
};
