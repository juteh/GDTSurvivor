#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GDTSurvivor/LevelUpOption.h"
#include "UpgradeSelectionWidget.generated.h"

class UCommonButtonBase;
class UTextBlock;
class UMetaProgressionSubsystem;

/**
 * Parent class of WBP_UpgradeSelection, the shop for permanent upgrades.
 * The widgets are bound by name from the designer; each row is a Buy button plus a text
 * "<Name> <Level>/<Max> Cost: <Cost>".
 */
UCLASS(Abstract)
class GDTSURVIVOR_API UUpgradeSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UUpgradeSelectionWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Label shown in front of the level, per upgrade type.
	UPROPERTY(EditDefaultsOnly, Category = "Upgrades")
	TMap<EUpgradeType, FText> UpgradeNames;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Money;

	// Firerate
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_1;

	// Health
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_2;

	// Shield
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_3;

	// Shield regeneration
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_4;

	// Damage
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_5;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_5;

	// Pickup range (magnet)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Buy_6;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Upgrade_6;

	// Refunds all permanent upgrades. Optional until it exists in the designer.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> WBP_ButtonBase_Reset;

private:
	struct FUpgradeRow
	{
		EUpgradeType Type;
		UCommonButtonBase* BuyButton;
		UTextBlock* Text;
	};

	UFUNCTION()
	void Refresh();

	void HandleBuyClicked(EUpgradeType Type);

	void HandleResetClicked();

	UMetaProgressionSubsystem* GetMetaProgression() const;

	TArray<FUpgradeRow> Rows;
};
