#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "UpgradeSelectionWidget.generated.h"

class UCommonButtonBase;
class UMetaProgressionSubsystem;
class UTextBlock;
class UUpgradeDefinition;

/**
 * Parent class of WBP_UpgradeSelection, the shop for permanent upgrades.
 * The designer has a fixed number of rows (Buy button + text, bound by name). They are filled in
 * order with the upgrades of the upgrade catalog that are sold as permanent; unused rows are hidden.
 * Each row shows "<Name> <Level>/<Max> Cost: <Cost>".
 */
UCLASS(Abstract)
class GDTSURVIVOR_API UUpgradeSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Money;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_5;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_5;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_6;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_6;

	// Refunds all permanent upgrades.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Reset;

private:
	struct FUpgradeRow
	{
		UCommonButtonBase* BuyButton = nullptr;
		UTextBlock* Text = nullptr;
		const UUpgradeDefinition* Upgrade = nullptr;
	};

	UFUNCTION()
	void Refresh();

	void HandleBuyClicked(int32 RowIndex);

	void HandleResetClicked();

	UMetaProgressionSubsystem* GetMetaProgression() const;

	TArray<FUpgradeRow> Rows;
};
