#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LevelUpSelectionWidget.generated.h"

class AShipPlayerState;
class UCommonButtonBase;
class UUpgradeDefinition;

/**
 * Parent class of WBP_LevelUpSelection. Rolls up to three level-up options from the owning
 * player's state, shows them on the option buttons and adds the chosen one to the player state
 * (the ship's stats follow from there). Closing and resuming the game is done by the owning
 * AShipPlayerController.
 */
UCLASS(Abstract)
class GDTSURVIVOR_API ULevelUpSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Upgrade_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Upgrade_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Upgrade_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Skip;

private:
	void HandleOptionClicked(int32 OptionIndex);

	void HandleSkipClicked();

	void FinishSelection();

	// Sets the visible text of a WBP_ButtonBase (its "ButtonBaseText" child).
	static void SetButtonLabel(UCommonButtonBase* Button, const FText& Label);

	TArray<UCommonButtonBase*> OptionButtons;

	UPROPERTY()
	TArray<TObjectPtr<UUpgradeDefinition>> Options;
};
