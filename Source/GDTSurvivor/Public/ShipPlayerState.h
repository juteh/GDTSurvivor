#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GDTSurvivor/LevelUpOption.h"
#include "ShipPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShipScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnShipExperienceChanged, int32, Experience, int32, ExperiencePerLevel, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShipLevelUp, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipUpgradesChanged);

/**
 * Parent class of BP_PlayerStateSpaceShip. Owns what the player has earned in the current run:
 * score, experience, level and the chosen level-up upgrades. Survives the pawn and is replicated,
 * so the HUD and the player list read it from here and listen to its events.
 * See Docs/Architecture.md.
 */
UCLASS()
class GDTSURVIVOR_API AShipPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AShipPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Ship player state of the actor's player (pawn or controller), or null.
	UFUNCTION(BlueprintPure, Category = "PlayerState", meta = (DisplayName = "Get Ship Player State"))
	static AShipPlayerState* GetShipPlayerState(const AActor* PlayerActor);

	// Score

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void AddScore(int32 Amount);

	// Restores the score carried over from the previous campaign level ("state" save).
	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void RestoreScoreFromRunState(int32 SavedScore);

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	int32 GetScoreAsInt() const { return FMath::RoundToInt(GetScore()); }

	// Experience and level

	// Adds experience and raises the level for every full ExperiencePerLevel. Leftover experience is kept.
	// Returns true if at least one level was gained.
	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	bool AddExperience(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	int32 GetExperience() const { return Experience; }

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	int32 GetExperiencePerLevel() const { return ExperiencePerLevel; }

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	int32 GetPlayerLevel() const { return Level; }

	// Level-up upgrades. Type is an EUpgradeType value as byte, because the level-up
	// Blueprints in BP_GameMode_Base pass upgrade types as byte.

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void AddUpgradeStack(uint8 Type);

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	int32 GetUpgradeStackCount(uint8 Type) const;

	// Events

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipLevelUp OnLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipUpgradesChanged OnUpgradesChanged;

protected:
	virtual void OnRep_Score() override;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerState", meta = (ClampMin = "1"))
	int32 ExperiencePerLevel = 10;

private:
	void BroadcastExperience();

	UFUNCTION()
	void OnRep_Experience();

	UFUNCTION()
	void OnRep_UpgradeStacks();

	UPROPERTY(ReplicatedUsing = OnRep_Experience)
	int32 Experience = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Experience)
	int32 Level = 1;

	// Stack count per EUpgradeType (index = enum value). An array because TMap can't replicate.
	UPROPERTY(ReplicatedUsing = OnRep_UpgradeStacks)
	TArray<int32> UpgradeStacks;
};
