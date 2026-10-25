#include "UI/GDTLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Game/GDTSurvivorSettings.h"
#include "MoviePlayer.h"

void UGDTLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IsRunningDedicatedServer() || !IsMoviePlayerEnabled() || !GetMoviePlayer())
	{
		return;
	}

	// The movie player starts a loading screen on every map load and asks for its content right before.
	PrepareHandle = GetMoviePlayer()->OnPrepareLoadingScreen().AddUObject(this, &UGDTLoadingScreenSubsystem::HandlePrepareLoadingScreen);
	FinishedHandle = GetMoviePlayer()->OnMoviePlaybackFinished().AddUObject(this, &UGDTLoadingScreenSubsystem::HandleLoadingScreenFinished);
}

void UGDTLoadingScreenSubsystem::Deinitialize()
{
	if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
	{
		MoviePlayer->OnPrepareLoadingScreen().Remove(PrepareHandle);
		MoviePlayer->OnMoviePlaybackFinished().Remove(FinishedHandle);
	}
	LoadingWidget = nullptr;

	Super::Deinitialize();
}

void UGDTLoadingScreenSubsystem::HandlePrepareLoadingScreen()
{
	const UGDTSurvivorSettings* ProjectSettings = GetDefault<UGDTSurvivorSettings>();
	const TSubclassOf<UUserWidget> WidgetClass = ProjectSettings->LoadingScreenWidget.LoadSynchronous();
	if (!WidgetClass)
	{
		return;
	}

	LoadingWidget = CreateWidget<UUserWidget>(GetGameInstance(), WidgetClass);
	if (!LoadingWidget)
	{
		return;
	}

	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.MinimumLoadingScreenDisplayTime = ProjectSettings->MinimumLoadingScreenTime;
	Attributes.WidgetLoadingScreen = LoadingWidget->TakeWidget();
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UGDTLoadingScreenSubsystem::HandleLoadingScreenFinished()
{
	LoadingWidget = nullptr;
}
