#include "UI/UpgradeEntryWidget.h"

#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "UpgradeEntry"

void UUpgradeEntryWidget::SetEntry(const FText& Name, int32 Stacks)
{
	CommonTextBlock_UpgradeText->SetText(FText::Format(LOCTEXT("Entry", "{0} LVL {1}"), Name, Stacks));
}

#undef LOCTEXT_NAMESPACE
