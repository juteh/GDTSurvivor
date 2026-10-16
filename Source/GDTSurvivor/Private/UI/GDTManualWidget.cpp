#include "UI/GDTManualWidget.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Input/CommonUIInputTypes.h"

#define LOCTEXT_NAMESPACE "ManualWidget"

UGDTManualWidget::UGDTManualWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Escape / gamepad back closes the manual.
	bIsBackHandler = true;
}

void UGDTManualWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Previous->OnClicked().AddUObject(this, &UGDTManualWidget::ShowPreviousPage);
	Button_Next->OnClicked().AddUObject(this, &UGDTManualWidget::HandleNextClicked);
	if (Button_Close)
	{
		Button_Close->OnClicked().AddUObject(this, &UGDTManualWidget::DeactivateWidget);
	}

	// CommonUI only routes these while the widget is activated.
	if (!PreviousPageInputAction.IsNull())
	{
		RegisterUIActionBinding(FBindUIActionArgs(PreviousPageInputAction, false,
			FSimpleDelegate::CreateUObject(this, &UGDTManualWidget::ShowPreviousPage)));
	}
	if (!NextPageInputAction.IsNull())
	{
		RegisterUIActionBinding(FBindUIActionArgs(NextPageInputAction, false,
			FSimpleDelegate::CreateUObject(this, &UGDTManualWidget::ShowNextPage)));
	}
}

void UGDTManualWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// The menu stack pools widgets, so a reopened manual would otherwise start on the last page seen.
	ShowPage(0);
}

UWidget* UGDTManualWidget::NativeGetDesiredFocusTarget() const
{
	return Button_Next;
}

void UGDTManualWidget::ShowPage(int32 PageIndex)
{
	const int32 PageCount = GetPageCount();
	if (PageCount == 0)
	{
		return;
	}

	PageSwitcher->SetActiveWidgetIndex(FMath::Clamp(PageIndex, 0, PageCount - 1));
	UpdatePageControls();
	OnPageChanged(GetPageIndex());
}

void UGDTManualWidget::ShowNextPage()
{
	if (GetPageIndex() < GetPageCount() - 1)
	{
		ShowPage(GetPageIndex() + 1);
	}
}

void UGDTManualWidget::ShowPreviousPage()
{
	if (GetPageIndex() > 0)
	{
		ShowPage(GetPageIndex() - 1);
	}
}

int32 UGDTManualWidget::GetPageIndex() const
{
	return PageSwitcher->GetActiveWidgetIndex();
}

int32 UGDTManualWidget::GetPageCount() const
{
	return PageSwitcher->GetNumWidgets();
}

void UGDTManualWidget::HandleNextClicked()
{
	if (GetPageIndex() >= GetPageCount() - 1)
	{
		DeactivateWidget();
		return;
	}
	ShowNextPage();
}

void UGDTManualWidget::UpdatePageControls()
{
	const int32 PageIndex = GetPageIndex();
	const int32 PageCount = GetPageCount();

	Button_Previous->SetIsEnabled(PageIndex > 0);

	if (Text_PageNumber)
	{
		Text_PageNumber->SetText(FText::Format(LOCTEXT("PageNumber", "{0} / {1}"), PageIndex + 1, PageCount));
	}
}

#undef LOCTEXT_NAMESPACE
