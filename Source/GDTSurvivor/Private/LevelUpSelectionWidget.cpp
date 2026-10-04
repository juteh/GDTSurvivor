#include "LevelUpSelectionWidget.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "ShipPlayerState.h"
#include "UpgradeDefinition.h"

void ULevelUpSelectionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	OptionButtons = {WBP_ButtonBase_Upgrade_1, WBP_ButtonBase_Upgrade_2, WBP_ButtonBase_Upgrade_3};
	for (int32 Index = 0; Index < OptionButtons.Num(); ++Index)
	{
		OptionButtons[Index]->OnClicked().AddWeakLambda(this, [this, Index]() { HandleOptionClicked(Index); });
	}

	if (WBP_ButtonBase_Skip)
	{
		WBP_ButtonBase_Skip->OnClicked().AddUObject(this, &ULevelUpSelectionWidget::HandleSkipClicked);
	}
}

void ULevelUpSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Options.Reset();
	if (const AShipPlayerState* PlayerState = GetOwningPlayerState<AShipPlayerState>())
	{
		Options.Append(PlayerState->GetLevelUpOptions(OptionButtons.Num()));
	}

	for (int32 Index = 0; Index < OptionButtons.Num(); ++Index)
	{
		UCommonButtonBase* Button = OptionButtons[Index];
		if (!Options.IsValidIndex(Index))
		{
			Button->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		Button->SetVisibility(ESlateVisibility::Visible);
		SetButtonLabel(Button, Options[Index]->DisplayName);
		Button->SetToolTipText(Options[Index]->Description);
	}
}

void ULevelUpSelectionWidget::HandleOptionClicked(int32 OptionIndex)
{
	AShipPlayerState* PlayerState = GetOwningPlayerState<AShipPlayerState>();
	if (PlayerState && Options.IsValidIndex(OptionIndex))
	{
		PlayerState->AddUpgradeStack(Options[OptionIndex]);
	}
	OnSelectionFinished();
}

void ULevelUpSelectionWidget::HandleSkipClicked()
{
	OnSelectionFinished();
}

void ULevelUpSelectionWidget::SetButtonLabel(UCommonButtonBase* Button, const FText& Label)
{
	if (UTextBlock* Text = Cast<UTextBlock>(Button->GetWidgetFromName(TEXT("ButtonBaseText"))))
	{
		Text->SetText(Label);
	}
}
