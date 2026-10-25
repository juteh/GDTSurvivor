#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GDTLoadingScreenSubsystem.generated.h"

class UUserWidget;

/**
 * Shows the loading screen on every map change (OpenLevel, travel). Loading a map blocks the game thread,
 * so the widget is handed to the engine's movie player, which draws it on its own thread until the new
 * map is loaded. Widget and minimum display time are set in Project Settings > Game > GDTSurvivor.
 * Only works in Standalone and packaged builds; the movie player is disabled in PIE.
 * See Docs/Architecture.md, section 2.
 */
UCLASS()
class GDTSURVIVOR_API UGDTLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandlePrepareLoadingScreen();
	void HandleLoadingScreenFinished();

	// Kept alive while the movie player shows it.
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingWidget;

	FDelegateHandle PrepareHandle;
	FDelegateHandle FinishedHandle;
};
