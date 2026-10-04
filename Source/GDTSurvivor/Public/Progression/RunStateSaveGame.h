#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RunStateSaveGame.generated.h"

/**
 * The current campaign run, carried from level to level: player name, unlocked levels and what the
 * player takes into the next level. Read and written only through URunStateSubsystem.
 */
UCLASS()
class GDTSURVIVOR_API URunStateSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FString PlayerName;

	UPROPERTY(SaveGame)
	int32 CompletedLevels = 0;

	UPROPERTY(SaveGame)
	int32 Score = 0;

	// Fill ratio (0..1) of health and shield at the end of the last completed level. A ratio instead of
	// absolute values, so upgrades that change the maximum don't add up across levels.
	UPROPERTY(SaveGame)
	float HealthFraction = 1.f;

	UPROPERTY(SaveGame)
	float ShieldFraction = 1.f;
};
