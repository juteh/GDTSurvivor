#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PlayerHUDWidget.generated.h"

class AShipPlayerState;
class APawn;
class APlayerSpaceShipPawn;
class UPanelWidget;
class UProgressBar;
class UShipStatsComponent;
class UTextBlock;
class UUpgradeEntryWidget;

/**
 * Parent class of WBP_HUD. Shows the player's values by listening to events, so nobody has to
 * push values into the HUD from outside:
 * - health and shield from the ship's UShipStatsComponent (follows the pawn on possession),
 * - score, level, experience and chosen upgrades from AShipPlayerState.
 */
UCLASS(Abstract)
class GDTSURVIVOR_API UPlayerHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// The player switched weapons; the Blueprint swaps the weapon icon.
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnSelectedWeaponChanged(const FString& WeaponName);

	// One line per chosen upgrade in VerticalBox_UpgradeEntries.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UUpgradeEntryWidget> UpgradeEntryClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> PROG_Health;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Health;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> PROG_Shield;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Shield;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_Score;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_LVL;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_EXP;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Experience;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> VerticalBox_UpgradeEntries;

private:
	// Ship stats (health, shield)

	void HandlePawnChanged(APawn* NewPawn);

	UFUNCTION()
	void HandleWeaponChanged(const FString& WeaponName);

	TWeakObjectPtr<APlayerSpaceShipPawn> BoundShip;

	void BindToStats(UShipStatsComponent* Stats);
	void UnbindFromStats();

	UFUNCTION()
	void HandleHealthChanged(float Current, float Max);

	UFUNCTION()
	void HandleShieldChanged(float Current, float Max);

	static void ShowResource(UProgressBar* Bar, UTextBlock* Text, float Current, float Max);

	// Player state (score, level, upgrades)

	void BindToPlayerState(AShipPlayerState* PlayerState);
	void UnbindFromPlayerState();

	UFUNCTION()
	void HandleScoreChanged(int32 NewScore);

	UFUNCTION()
	void HandleExperienceChanged(int32 Experience, int32 ExperiencePerLevel, int32 Level);

	UFUNCTION()
	void HandleUpgradesChanged();

	TWeakObjectPtr<UShipStatsComponent> BoundStats;

	TWeakObjectPtr<AShipPlayerState> BoundPlayerState;

	FDelegateHandle PawnChangedHandle;
};
