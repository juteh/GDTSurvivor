#include "UpgradeSelectionWidget.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "MetaProgressionSubsystem.h"
#include "UpgradeCatalog.h"
#include "UpgradeDefinition.h"

#define LOCTEXT_NAMESPACE "UpgradeSelection"

void UUpgradeSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Rows = {
		{WBP_ButtonBase_Buy_1, CommonTextBlock_Upgrade_1},
		{WBP_ButtonBase_Buy_2, CommonTextBlock_Upgrade_2},
		{WBP_ButtonBase_Buy_3, CommonTextBlock_Upgrade_3},
		{WBP_ButtonBase_Buy_4, CommonTextBlock_Upgrade_4},
		{WBP_ButtonBase_Buy_5, CommonTextBlock_Upgrade_5},
		{WBP_ButtonBase_Buy_6, CommonTextBlock_Upgrade_6},
	};

	// Fill the rows in catalog order with the upgrades that are sold as permanent.
	int32 RowIndex = 0;
	if (const UUpgradeCatalog* Catalog = UUpgradeCatalog::Get())
	{
		for (const UUpgradeDefinition* Upgrade : Catalog->Upgrades)
		{
			if (Upgrade && Upgrade->bSellAsPermanent && Rows.IsValidIndex(RowIndex))
			{
				Rows[RowIndex++].Upgrade = Upgrade;
			}
		}
	}

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const FUpgradeRow& Row = Rows[Index];
		if (!Row.Upgrade)
		{
			Row.BuyButton->SetVisibility(ESlateVisibility::Collapsed);
			Row.Text->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Row.BuyButton->OnClicked().AddWeakLambda(this, [this, Index]() { HandleBuyClicked(Index); });
	}

	if (WBP_ButtonBase_Reset)
	{
		WBP_ButtonBase_Reset->OnClicked().AddUObject(this, &UUpgradeSelectionWidget::HandleResetClicked);
	}
}

void UUpgradeSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMetaProgressionSubsystem* MetaProgression = GetMetaProgression())
	{
		MetaProgression->OnMetaProgressionChanged.AddUniqueDynamic(this, &UUpgradeSelectionWidget::Refresh);
	}
	Refresh();
}

void UUpgradeSelectionWidget::NativeDestruct()
{
	if (UMetaProgressionSubsystem* MetaProgression = GetMetaProgression())
	{
		MetaProgression->OnMetaProgressionChanged.RemoveDynamic(this, &UUpgradeSelectionWidget::Refresh);
	}

	Super::NativeDestruct();
}

void UUpgradeSelectionWidget::Refresh()
{
	const UMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	if (!MetaProgression)
	{
		return;
	}

	CommonTextBlock_Money->SetText(FText::Format(LOCTEXT("Money", "Money: {0}"), MetaProgression->GetMoney()));

	for (const FUpgradeRow& Row : Rows)
	{
		if (!Row.Upgrade)
		{
			continue;
		}

		const int32 Level = MetaProgression->GetUpgradeLevel(Row.Upgrade);
		const int32 MaxLevel = Row.Upgrade->MaxPermanentLevel;

		const FText Text = Level >= MaxLevel
			? FText::Format(LOCTEXT("RowMaxed", "{0} {1}/{2} MAX"), Row.Upgrade->DisplayName, Level, MaxLevel)
			: FText::Format(LOCTEXT("Row", "{0} {1}/{2} Cost: {3}"), Row.Upgrade->DisplayName, Level, MaxLevel, Row.Upgrade->PermanentCost);
		Row.Text->SetText(Text);

		Row.BuyButton->SetIsEnabled(MetaProgression->CanBuyUpgrade(Row.Upgrade));
	}
}

void UUpgradeSelectionWidget::HandleBuyClicked(int32 RowIndex)
{
	UMetaProgressionSubsystem* MetaProgression = GetMetaProgression();
	if (MetaProgression && Rows.IsValidIndex(RowIndex))
	{
		MetaProgression->BuyUpgrade(Rows[RowIndex].Upgrade);
	}
}

void UUpgradeSelectionWidget::HandleResetClicked()
{
	if (UMetaProgressionSubsystem* MetaProgression = GetMetaProgression())
	{
		MetaProgression->ResetUpgrades();
	}
}

UMetaProgressionSubsystem* UUpgradeSelectionWidget::GetMetaProgression() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMetaProgressionSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
