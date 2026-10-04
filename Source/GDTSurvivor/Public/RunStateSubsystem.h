#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RunStateSubsystem.generated.h"

class URunStateSaveGame;

/**
 * Owns the current campaign run (save slot "runstate"): player name, completed levels, and the score,
 * health and shield carried into the next level. Menus start and continue runs through it;
 * AShipPlayerState and UShipStatsComponent read their start values from it once when they begin play.
 * See Docs/Architecture.md, section 4.
 */
UCLASS()
class GDTSURVIVOR_API URunStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Shortcut for Blueprints and C++ code that only has a world context.
	UFUNCTION(BlueprintPure, Category = "RunState", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Run State"))
	static URunStateSubsystem* GetRunState(const UObject* WorldContextObject);

	// True if a run was started (the menu's "Continue" is available).
	UFUNCTION(BlueprintPure, Category = "RunState")
	bool HasRun() const;

	// Replaces any existing run with a fresh one.
	UFUNCTION(BlueprintCallable, Category = "RunState")
	void StartNewRun();

	UFUNCTION(BlueprintCallable, Category = "RunState")
	void DeleteRun();

	UFUNCTION(BlueprintPure, Category = "RunState")
	FString GetPlayerName() const;

	UFUNCTION(BlueprintCallable, Category = "RunState")
	void SetPlayerName(const FString& PlayerName);

	UFUNCTION(BlueprintPure, Category = "RunState")
	int32 GetCompletedLevels() const;

	UFUNCTION(BlueprintPure, Category = "RunState")
	int32 GetScore() const;

	UFUNCTION(BlueprintPure, Category = "RunState")
	float GetHealthFraction() const;

	UFUNCTION(BlueprintPure, Category = "RunState")
	float GetShieldFraction() const;

	// Stores what the player takes into the next level and unlocks it.
	UFUNCTION(BlueprintCallable, Category = "RunState")
	void RecordLevelCompleted(int32 Score, float HealthFraction, float ShieldFraction);

private:
	void Save();

	UPROPERTY()
	TObjectPtr<URunStateSaveGame> SaveGame;

	static const FString SaveSlotName;
};
