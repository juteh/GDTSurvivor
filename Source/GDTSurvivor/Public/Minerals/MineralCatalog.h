#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MineralCatalog.generated.h"

class UMineralDefinition;

// Size classes of mineral asteroids, smallest first (spawn markers allow a range of them).
UENUM(BlueprintType)
enum class EMineralAsteroidSize : uint8
{
	Small,
	Medium,
	Large,
	Huge
};

// What a size class means for the asteroid.
USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FMineralAsteroidSizeStats
{
	GENERATED_BODY()

	// Actor scale (8 = mesh 8x the small mineral).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals", meta = (ClampMin = "0.1"))
	float Scale = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals", meta = (ClampMin = "1"))
	float Health = 200.0f;

	// Small minerals it breaks into.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals", meta = (ClampMin = "0"))
	int32 Minerals = 5;
};

/**
 * A ring around the base. A tile belongs to the zone whose MinDistanceInTiles is the highest one
 * not above the distance of the tile center to the base tile (straight line, in tiles).
 */
USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FMineralZone
{
	GENERATED_BODY()

	// Shown in the tile debug text (and later in the HUD).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals", meta = (ClampMin = "0"))
	float MinDistanceInTiles = 0.0f;

	// Relative chance of each mineral for asteroids in this zone. Missing = never.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals")
	TMap<TObjectPtr<UMineralDefinition>, float> MineralWeights;

	// Relative chance of each size class in this zone. Missing = never.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minerals")
	TMap<EMineralAsteroidSize, float> SizeWeights;
};

/**
 * All minerals, asteroid sizes and the zones around the base ("high risk, high reward").
 * Tiles decide where asteroids can be, the zone of the tile decides which mineral and size they get.
 * Set in Project Settings > Game > GDTSurvivor > Mineral Catalog.
 */
UCLASS(BlueprintType)
class GDTSURVIVOR_API UMineralCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UMineralCatalog();

	// The catalog from the project settings, or null (with a log error) if none is set.
	static const UMineralCatalog* Get();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minerals")
	TArray<TObjectPtr<UMineralDefinition>> Minerals;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minerals")
	TMap<EMineralAsteroidSize, FMineralAsteroidSizeStats> Sizes;

	// Sorted by MinDistanceInTiles, the first one starts at 0.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minerals")
	TArray<FMineralZone> Zones;

	// Zone of the tile at the given grid coordinate (base tile = 0, 0). Null if there are no zones.
	const FMineralZone* FindZone(const FIntPoint& TileCoord) const;

	// Weighted pick from the zone; null if the zone allows no mineral.
	const UMineralDefinition* PickMineral(const FMineralZone& Zone, FRandomStream& Stream) const;

	// Weighted pick from the zone, limited to MinSize..MaxSize. False if nothing in the range is allowed.
	bool PickSize(const FMineralZone& Zone, EMineralAsteroidSize MinSize, EMineralAsteroidSize MaxSize,
		FRandomStream& Stream, EMineralAsteroidSize& OutSize) const;

	// Equal chance for every size in MinSize..MaxSize, ignoring the zone (used by designer markers).
	// False only if no size in the range is defined in Sizes.
	bool PickSizeInRange(EMineralAsteroidSize MinSize, EMineralAsteroidSize MaxSize,
		FRandomStream& Stream, EMineralAsteroidSize& OutSize) const;

	const UMineralDefinition* FindById(FName MineralId) const;

	// The mineral whose asteroid breaks into pieces of this class.
	const UMineralDefinition* FindByMineralClass(const UClass* MineralClass) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
