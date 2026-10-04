#include "UpgradeSelectionWidget.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "MetaProgressionSubsystem.h"

#define LOCTEXT_NAMESPACE "UpgradeSelection"

UUpgradeSelectionWidget::UUpgradeSelectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UpgradeNames.Add(EUpgradeType::FireRate, LOCTEXT("FireRate", "Firerate"));
	UpgradeNames.Add(EUpgradeType::Health, LOCTEXT("Health", "Life"));
	UpgradeNames.Add(EUpgradeType::Shield, LOCTEXT("Shield", "Shield"));
	UpgradeNames.Add(EUpgradeType::ShieldRegen, LOCTEXT("ShieldRegen", "Shieldrate"));
	UpgradeNames.Add(EUpgradeType::Damage, LOCTEXT("Damage", "Damage"));
	UpgradeNames.Add(EUpgradeType::PickupRange, LOCTEXT("PickupRange", "Magnet"));
}

void UUpgradeSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Rows = {
		{EUpgradeType::FireRate, WBP_ButtonBase_Buy_1, CommonTextBlock_Upgrade_1},
		{EUpgradeType::Health, WBP_ButtonBase_Buy_2, CommonTextBlock_Upgrade_2},
		{EUpgradeType::Shield, WBP_ButtonBase_Buy_3, CommonTextBlock_Upgrade_3},
		{EUpgradeType::ShieldRegen, WBP_ButtonBase_Buy_4, CommonTextBlock_Upgrade_4},
		{EUpgradeType::Damage, WBP_ButtonBase_Buy_5, CommonTextBlock_Upgrade_5},
		{EUpgradeType::PickupRange, WBP_ButtonBase_Buy_6, CommonTextBlock_Upgrade_6},
	};

	for (const FUpgradeRow& Row : Rows)
	{
		const EUpgradeType Type = Row.Type;
		Row.BuyButton->OnClicked().AddWeakLambda(this, [this, Type]() { HandleBuyClicked(Type); });
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
		const int32 Level = MetaProgression->GetUpgradeLevel(Row.Type);
		const int32 MaxLevel = MetaProgression->GetMaxUpgradeLevel(Row.Type);
		const FText Name = UpgradeNames.FindRef(Row.Type);

		const FText Text = Level >= MaxLevel
			? FText::Format(LOCTEXT("RowMaxed", "{0} {1}/{2} MAX"), Name, Level, MaxLevel)
			: FText::Format(LOCTEXT("Row", "{0} {1}/{2} Cost: {3}"), Name, Level, MaxLevel, MetaProgression->GetUpgradeCost(Row.Type));
		Row.Text->SetText(Text);

		Row.BuyButton->SetIsEnabled(MetaProgression->CanBuyUpgrade(Row.Type));
	}
}

void UUpgradeSelectionWidget::HandleBuyClicked(EUpgradeType Type)
{
	if (UMetaProgressionSubsystem* MetaProgression = GetMetaProgression())
	{
		MetaProgression->BuyUpgrade(Type);
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
