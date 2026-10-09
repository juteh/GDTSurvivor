#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShipPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;

UENUM(BlueprintType)
enum class EMatchResult : uint8
{
	Victory,
	Defeat,
	// End of a timed match (endless mode): show the result board.
	Results
};

/**
 * Parent class of BP_SpaceShipPC. Owns everything the local player sees on screen: the HUD (which contains
 * the objective HUD), the tutorial overlay, the pause menu, the level-up selection (queued, pauses the game) and
 * the end-of-match screens. The GameMode only decides *that* a match ended and tells the controllers.
 * Also owns the interface input (pause, tutorial, player list), so it keeps working while the player
 * has no ship. Ship controls (move, shoot, weapons) stay on the pawn.
 * See Docs/Architecture.md, section 2.
 */
UCLASS()
class GDTSURVIVOR_API AShipPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// Called by the GameMode (server) when the match is decided.
	UFUNCTION(BlueprintCallable, Category = "Screens", meta = (WorldContext = "WorldContextObject"))
	static void ShowMatchResultForAllPlayers(const UObject* WorldContextObject, EMatchResult Result);

	UFUNCTION(Client, Reliable)
	void ClientShowMatchResult(EMatchResult Result);

	UFUNCTION(BlueprintCallable, Category = "Screens")
	void ShowPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Screens")
	void ToggleTutorial();

	// Called by the level-up selection after an option was chosen or skipped.
	UFUNCTION(BlueprintCallable, Category = "Screens")
	void CloseLevelUpSelection();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnRep_PlayerState() override;

	// Shows or hides the player list. Implemented in BP_SpaceShipPC, which still owns that widget.
	UFUNCTION(BlueprintImplementableEvent, Category = "Screens")
	void OnTogglePlayerList();

	// Interface input, added on top of the pawn's ship controls.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InterfaceMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 InterfaceMappingPriority = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleTutorialAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> TogglePlayerListAction;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> HUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> TutorialClass;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> PauseMenuClass;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> LevelUpSelectionClass;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> VictoryScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> DefeatScreenClass;

	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	TSubclassOf<UUserWidget> ResultBoardClass;

	// Draw order of the always-visible overlays (HUD, tutorial).
	UPROPERTY(EditDefaultsOnly, Category = "Screens")
	int32 OverlayZOrder = 2;

private:
	void HandleTogglePlayerList();

	void BindToPlayerState();

	UFUNCTION()
	void HandleLevelUp(int32 NewLevel);

	void ShowNextLevelUpSelection();

	// Adds the widget, pauses the game and gives the mouse to the UI.
	void ShowPausingScreen(UUserWidget* Screen, bool bUIOnly);

	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> TutorialWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> LevelUpWidget;

	// Level-ups that still wait for a selection (several can happen before the first one is closed).
	int32 PendingLevelUps = 0;

	bool bBoundToPlayerState = false;
};
