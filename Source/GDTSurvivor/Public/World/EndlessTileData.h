#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Minerals/MineralCatalog.h"
#include "EndlessTileData.generated.h"

class UMaterialInterface;
class UMineralDefinition;
class UStaticMesh;

// How mineral asteroids get into a tile. Which mineral and size they get is decided by the zone.
UENUM(BlueprintType)
enum class EMineralAsteroidPlacement : uint8
{
	// No asteroids at all (e.g. the start tile).
	None,
	// Only at the spawn markers placed in the tile map.
	Markers,
	// Only on random free spots.
	Random,
	// Markers first, then random ones on the remaining free spots.
	MarkersAndRandom
};

// A spot for a mineral asteroid, placed as AMineralAsteroidSpawnPoint in the tile map.
USTRUCT()
struct FEndlessTileAsteroidSlot
{
	GENERATED_BODY()

	// Relative to the tile center.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	float Chance = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	EMineralAsteroidSize MinSize = EMineralAsteroidSize::Small;

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	EMineralAsteroidSize MaxSize = EMineralAsteroidSize::Huge;

	// Null = picked by the zone.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TObjectPtr<UMineralDefinition> FixedMineral;
};

// All static meshes of one kind (same mesh, materials and collision) in a tile.
// Rendered as one instanced component at runtime.
USTRUCT()
struct FEndlessTileMeshGroup
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	FName CollisionProfile = TEXT("BlockAll");

	// Relative to the tile center.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TArray<FTransform> Instances;
};

// A gameplay actor (mineral, pickup, ...) that is spawned when the tile is placed.
USTRUCT()
struct FEndlessTileActorEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TSubclassOf<AActor> ActorClass;

	// Relative to the tile center.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	FTransform Transform;
};

/**
 * Baked content of one designer-made tile map (Tile_A, Tile_B, ...).
 * Do not edit by hand: change the tile map and press "Bake" on its EndlessTileAuthoring actor.
 */
UCLASS(BlueprintType)
class GDTSURVIVOR_API UEndlessTileData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	FString TileName;

	// May be placed rotated by 90/180/270 degrees.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	bool bAllowRotation = true;

	// Relative chance to be picked compared to the other tiles in the set.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	float Weight = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Asteroids")
	EMineralAsteroidPlacement AsteroidPlacement = EMineralAsteroidPlacement::Random;

	// Number of random asteroids (Random / MarkersAndRandom), picked between Min and Max.
	UPROPERTY(VisibleAnywhere, Category = "Tile|Asteroids")
	int32 MinRandomAsteroids = 0;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Asteroids")
	int32 MaxRandomAsteroids = 2;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TArray<FEndlessTileAsteroidSlot> AsteroidSlots;

	// Old switch, replaced by AsteroidPlacement. Only read in PostLoad: false becomes None.
	UPROPERTY()
	bool bAllowMineralAsteroids = true;

	virtual void PostLoad() override;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TSoftObjectPtr<UWorld> SourceMap;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TArray<FEndlessTileMeshGroup> MeshGroups;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TArray<FEndlessTileActorEntry> Actors;
};
