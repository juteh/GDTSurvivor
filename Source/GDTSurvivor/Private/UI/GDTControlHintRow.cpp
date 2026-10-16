#include "UI/GDTControlHintRow.h"

#include "Components/TextBlock.h"

void UGDTControlHintRow::SetLabel(const FText& InLabel)
{
	Label = InLabel;
	Text_Label->SetText(Label);
}

void UGDTControlHintRow::NativePreConstruct()
{
	Super::NativePreConstruct();

	Text_Label->SetText(Label);
}
