#include "Game/ShipPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/ShipPlayerState.h"

void AShipPlayerController::ShowMatchResultForAllPlayers(const UObject* WorldContextObject, EMatchResult Result)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShipPlayerController* PlayerController = Cast<AShipPlayerController>(It->Get()))
		{
			PlayerController->ClientShowMatchResult(Result);
		}
	}
}

void AShipPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (InterfaceMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(InterfaceMappingContext, InterfaceMappingPriority);
		}
	}

	if (HUDClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDClass);
		HUDWidget->AddToViewport(OverlayZOrder);
	}
	if (TutorialClass)
	{
		TutorialWidget = CreateWidget<UUserWidget>(this, TutorialClass);
		TutorialWidget->AddToViewport(OverlayZOrder);
	}

	BindToPlayerState();
}

void AShipPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	// Started = once per key press, no matter which triggers the action has.
	if (PauseAction)
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AShipPlayerController::ShowPauseMenu);
	}
	if (ToggleTutorialAction)
	{
		EnhancedInput->BindAction(ToggleTutorialAction, ETriggerEvent::Started, this, &AShipPlayerController::ToggleTutorial);
	}
	if (TogglePlayerListAction)
	{
		EnhancedInput->BindAction(TogglePlayerListAction, ETriggerEvent::Started, this, &AShipPlayerController::HandleTogglePlayerList);
	}
}

void AShipPlayerController::HandleTogglePlayerList()
{
	OnTogglePlayerList();
}

void AShipPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BindToPlayerState();
}

void AShipPlayerController::BindToPlayerState()
{
	AShipPlayerState* ShipPlayerState = GetPlayerState<AShipPlayerState>();
	if (bBoundToPlayerState || !ShipPlayerState || !IsLocalController())
	{
		return;
	}

	ShipPlayerState->OnLevelUp.AddUniqueDynamic(this, &AShipPlayerController::HandleLevelUp);
	bBoundToPlayerState = true;
}

void AShipPlayerController::HandleLevelUp(int32 NewLevel)
{
	++PendingLevelUps;
	if (!LevelUpWidget)
	{
		ShowNextLevelUpSelection();
	}
}

void AShipPlayerController::ShowNextLevelUpSelection()
{
	if (!LevelUpSelectionClass)
	{
		PendingLevelUps = 0;
		return;
	}

	LevelUpWidget = CreateWidget<UUserWidget>(this, LevelUpSelectionClass);
	ShowPausingScreen(LevelUpWidget, false);
}

void AShipPlayerController::CloseLevelUpSelection()
{
	if (LevelUpWidget)
	{
		LevelUpWidget->RemoveFromParent();
		LevelUpWidget = nullptr;
	}

	PendingLevelUps = FMath::Max(0, PendingLevelUps - 1);
	if (PendingLevelUps > 0)
	{
		ShowNextLevelUpSelection();
		return;
	}

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void AShipPlayerController::ShowPauseMenu()
{
	if (PauseMenuClass)
	{
		ShowPausingScreen(CreateWidget<UUserWidget>(this, PauseMenuClass), false);
	}
}

void AShipPlayerController::ToggleTutorial()
{
	if (TutorialWidget)
	{
		TutorialWidget->SetVisibility(TutorialWidget->IsVisible() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
	}
}

void AShipPlayerController::ClientShowMatchResult_Implementation(EMatchResult Result)
{
	TSubclassOf<UUserWidget> ScreenClass;
	switch (Result)
	{
	case EMatchResult::Victory: ScreenClass = VictoryScreenClass; break;
	case EMatchResult::Defeat:  ScreenClass = DefeatScreenClass; break;
	case EMatchResult::Results: ScreenClass = ResultBoardClass; break;
	}
	if (!ScreenClass)
	{
		return;
	}

	// Defeat and results replace everything on screen; victory is shown on top of the HUD.
	if (Result != EMatchResult::Victory)
	{
		UWidgetLayoutLibrary::RemoveAllWidgets(this);
		HUDWidget = nullptr;
		TutorialWidget = nullptr;
		LevelUpWidget = nullptr;
		PendingLevelUps = 0;
	}

	ShowPausingScreen(CreateWidget<UUserWidget>(this, ScreenClass), true);
}

void AShipPlayerController::ShowPausingScreen(UUserWidget* Screen, bool bUIOnly)
{
	Screen->AddToViewport();
	SetPause(true);
	SetShowMouseCursor(true);

	if (bUIOnly)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Screen->TakeWidget());
		SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(Screen->TakeWidget());
		SetInputMode(InputMode);
		// Keeps keyboard input (e.g. the pause key) working while the screen is open, like before.
		UWidgetBlueprintLibrary::SetFocusToGameViewport();
	}
}
