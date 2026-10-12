#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MineralDefinition.generated.h"

class AMineralAsteroid;
class UTexture2D;

/**
 * One mineral type (Asteronium, Crystaline, Iconos, ...). Listed in the mineral catalog.
 * Which mineral shows up where is decided by the catalog's zones, what it is worth only
 * matters in the evaluation at the end of a level.
 */
UCLASS(BlueprintType)
class GDTSURVIVOR_API UMineralDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Stable id, e.g. for counting collected minerals. Never change it once it is in use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mineral")
	FName MineralId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mineral")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mineral")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mineral")
	TObjectPtr<UTexture2D> Icon;

	// Points per collected mineral in the evaluation at the end of the level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mineral", meta = (ClampMin = "0"))
	int32 Value = 1;

	// Big rock of this mineral (BP_MineralAsteroid_Iconos, ...). Its MineralClass is the small
	// piece it breaks into, so that is not repeated here.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mineral")
	TSubclassOf<AMineralAsteroid> AsteroidClass;

	// Small collectable piece (from the asteroid class), or null.
	TSubclassOf<AActor> GetMineralClass() const;
};

// How many pieces of one mineral a player has (carried or delivered).
USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FMineralCount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mineral")
	TObjectPtr<UMineralDefinition> Mineral;

	UPROPERTY(BlueprintReadOnly, Category = "Mineral")
	int32 Count = 0;
};
