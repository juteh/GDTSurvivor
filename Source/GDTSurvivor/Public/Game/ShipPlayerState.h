#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Minerals/MineralDefinition.h"
#include "ShipPlayerState.generated.h"

class UUpgradeDefinition;

// How often the player chose one upgrade as level-up in this run.
USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FUpgradeStack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<UUpgradeDefinition> Upgrade;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	int32 Stacks = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShipScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnShipExperienceChanged, int32, Experience, int32, ExperiencePerLevel, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShipLevelUp, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipUpgradesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipMineralsChanged);

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Ship player state of the actor's player (pawn or controller), or null.
	UFUNCTION(BlueprintPure, Category = "PlayerState", meta = (DisplayName = "Get Ship Player State"))
	static AShipPlayerState* GetShipPlayerState(const AActor* PlayerActor);

	// Score

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void AddScore(int32 Amount);

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

	// Level-up upgrades

	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	void AddUpgradeStack(const UUpgradeDefinition* Upgrade);

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	int32 GetUpgradeStackCount(const UUpgradeDefinition* Upgrade) const;

	UFUNCTION(BlueprintPure, Category = "PlayerState")
	const TArray<FUpgradeStack>& GetUpgradeStacks() const { return UpgradeStacks; }

	// Up to Count random different upgrades that can still be chosen as level-up.
	UFUNCTION(BlueprintCallable, Category = "PlayerState")
	TArray<UUpgradeDefinition*> GetLevelUpOptions(int32 Count) const;

	// Minerals: picked up pieces are carried until the player reaches the dropzone, then they count
	// as delivered. Only which minerals were delivered is counted; their value matters in the
	// evaluation at the end of the level (see Docs/Architecture.md, section 5b).

	// Server: the player picked up this mineral piece (BP_Mineral_*). False if it is not in the
	// mineral catalog.
	UFUNCTION(BlueprintCallable, Category = "PlayerState|Minerals")
	bool CollectMineral(const AActor* MineralActor);

	// Server: everything carried counts as delivered. Returns the number of delivered pieces.
	UFUNCTION(BlueprintCallable, Category = "PlayerState|Minerals")
	int32 DeliverMinerals();

	UFUNCTION(BlueprintPure, Category = "PlayerState|Minerals")
	const TArray<FMineralCount>& GetCarriedMinerals() const { return CarriedMinerals; }

	UFUNCTION(BlueprintPure, Category = "PlayerState|Minerals")
	const TArray<FMineralCount>& GetDeliveredMinerals() const { return DeliveredMinerals; }

	// Delivered pieces of one mineral, 0 if it was never delivered.
	UFUNCTION(BlueprintPure, Category = "PlayerState|Minerals")
	int32 GetDeliveredMineralCount(const UMineralDefinition* Mineral) const;

	// Sum of count * value of all delivered minerals, for the evaluation.
	UFUNCTION(BlueprintPure, Category = "PlayerState|Minerals")
	int32 GetDeliveredMineralValue() const;

	// Events

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipExperienceChanged OnExperienceChanged;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipLevelUp OnLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipUpgradesChanged OnUpgradesChanged;

	// Carried or delivered minerals changed.
	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnShipMineralsChanged OnMineralsChanged;

protected:
	// Takes the score carried over from the previous campaign level (URunStateSubsystem).
	virtual void BeginPlay() override;

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

	UPROPERTY(ReplicatedUsing = OnRep_UpgradeStacks)
	TArray<FUpgradeStack> UpgradeStacks;

	UFUNCTION()
	void OnRep_Minerals();

	UPROPERTY(ReplicatedUsing = OnRep_Minerals)
	TArray<FMineralCount> CarriedMinerals;

	UPROPERTY(ReplicatedUsing = OnRep_Minerals)
	TArray<FMineralCount> DeliveredMinerals;
};
