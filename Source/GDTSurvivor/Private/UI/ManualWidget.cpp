#include "UI/ManualWidget.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Input/CommonUIInputTypes.h"

#define LOCTEXT_NAMESPACE "ManualWidget"

UManualWidget::UManualWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Escape / gamepad back closes the manual.
	bIsBackHandler = true;
}

void UManualWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Button_Previous->OnClicked().AddUObject(this, &UManualWidget::ShowPreviousPage);
	Button_Next->OnClicked().AddUObject(this, &UManualWidget::HandleNextClicked);
	if (Button_Close)
	{
		Button_Close->OnClicked().AddUObject(this, &UManualWidget::DeactivateWidget);
	}

	// CommonUI only routes these while the widget is activated.
	if (!PreviousPageInputAction.IsNull())
	{
		RegisterUIActionBinding(FBindUIActionArgs(PreviousPageInputAction, false,
			FSimpleDelegate::CreateUObject(this, &UManualWidget::ShowPreviousPage)));
	}
	if (!NextPageInputAction.IsNull())
	{
		RegisterUIActionBinding(FBindUIActionArgs(NextPageInputAction, false,
			FSimpleDelegate::CreateUObject(this, &UManualWidget::ShowNextPage)));
	}
}

void UManualWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// The menu stack pools widgets, so a reopened manual would otherwise start on the last page seen.
	ShowPage(0);
}

UWidget* UManualWidget::NativeGetDesiredFocusTarget() const
{
	return Button_Next;
}

void UManualWidget::ShowPage(int32 PageIndex)
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

void UManualWidget::ShowNextPage()
{
	if (GetPageIndex() < GetPageCount() - 1)
	{
		ShowPage(GetPageIndex() + 1);
	}
}

void UManualWidget::ShowPreviousPage()
{
	if (GetPageIndex() > 0)
	{
		ShowPage(GetPageIndex() - 1);
	}
}

int32 UManualWidget::GetPageIndex() const
{
	return PageSwitcher->GetActiveWidgetIndex();
}

int32 UManualWidget::GetPageCount() const
{
	return PageSwitcher->GetNumWidgets();
}

void UManualWidget::HandleNextClicked()
{
	if (GetPageIndex() >= GetPageCount() - 1)
	{
		DeactivateWidget();
		return;
	}
	ShowNextPage();
}

void UManualWidget::UpdatePageControls()
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
