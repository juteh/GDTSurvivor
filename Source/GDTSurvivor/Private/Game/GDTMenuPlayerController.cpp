#include "Game/GDTMenuPlayerController.h"

#include "Blueprint/UserWidget.h"

void AGDTMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !MenuWidgetClass)
	{
		return;
	}

	MenuWidget = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	MenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}
