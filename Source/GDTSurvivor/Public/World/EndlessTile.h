#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndlessTile.generated.h"

class AMineralAsteroid;
class UEndlessTileData;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;

// One size class of mineral asteroids.
USTRUCT(BlueprintType)
struct FMineralAsteroidSize
{
	GENERATED_BODY()

	// Actor scale (8 = mesh 8x the small mineral).
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0.1"))
	float Scale = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "1"))
	float Health = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0"))
	int32 Minerals = 5;

	// Relative chance at the base tile ...
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0"))
	float NearWeight = 1.0f;

	// ... and at FarDistanceInTiles or further. Linearly blended in between.
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0"))
	float FarWeight = 1.0f;
};

// How random mineral asteroids are added to tiles (set on the EndlessTileManager).
USTRUCT(BlueprintType)
struct FMineralAsteroidSpawnSettings
{
	GENERATED_BODY()

	FMineralAsteroidSpawnSettings()
	{
		auto AddSize = [this](float Scale, float Health, int32 Minerals, float NearWeight, float FarWeight)
		{
			FMineralAsteroidSize& Size = Sizes.AddDefaulted_GetRef();
			Size.Scale = Scale;
			Size.Health = Health;
			Size.Minerals = Minerals;
			Size.NearWeight = NearWeight;
			Size.FarWeight = FarWeight;
		};
		AddSize(2.0f, 50.0f, 2, 50.0f, 5.0f);
		AddSize(4.0f, 100.0f, 3, 30.0f, 15.0f);
		AddSize(6.0f, 150.0f, 4, 15.0f, 30.0f);
		AddSize(8.0f, 200.0f, 5, 5.0f, 50.0f);
	}

	// Size classes; the further a tile is from the base, the more FarWeight counts.
	UPROPERTY(EditAnywhere, Category = "Asteroids")
	TArray<FMineralAsteroidSize> Sizes;

	// Distance from the base tile (in tiles) at which only FarWeight is used.
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "1"))
	float FarDistanceInTiles = 10.0f;

	// One is picked at random per asteroid.
	UPROPERTY(EditAnywhere, Category = "Asteroids")
	TArray<TSubclassOf<AMineralAsteroid>> Classes;

	// Chance for the first asteroid in a tile. Every further one needs another roll with the
	// same chance (0.5: 50% one, 25% two, ...).
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0"))
	int32 MaxPerTile = 2;

	// Border strip kept free, like the EdgeMargin in the tile maps.
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0"))
	float EdgeMargin = 500.0f;

	// Minimum free gap between an asteroid and anything else in the tile.
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "0"))
	float Clearance = 300.0f;

	// Tries per asteroid to find a free spot before giving up.
	UPROPERTY(EditAnywhere, Category = "Asteroids", meta = (ClampMin = "1"))
	int32 PlacementAttempts = 20;
};

/**
 * One square chunk of the endless map. Owned and recycled by AEndlessTileManager:
 * when the player moves away, the tile is moved to a new grid coordinate and
 * re-filled via InitTile() with the content of a designer-made tile (UEndlessTileData).
 */
UCLASS()
class GDTSURVIVOR_API AEndlessTile : public AActor
{
	GENERATED_BODY()

public:
	AEndlessTile();

	// Moves the tile to the given grid coordinate (tile (0,0) is centered on the world origin)
	// and rebuilds it from Data, rotated by RotationSteps * 90 degrees. Data may be null (empty tile).
	// Asteroid placement is deterministic per coordinate and Seed.
	void InitTile(const FIntPoint& InCoord, float InTileSize, const UEndlessTileData* Data,
		int32 RotationSteps, const FMineralAsteroidSpawnSettings& AsteroidSettings, int32 Seed, bool bShowDebug);

	FIntPoint GetCoord() const { return Coord; }

	// Invisible floor, only there so the NavMesh has something to generate on.
	// Same collision setup as the "groundplane" in the prototype levels (blocks Pawn only).
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TObjectPtr<UStaticMeshComponent> NavFloor;

	// Debug: tile name, grid coordinate and rotation in the tile center.
	UPROPERTY(VisibleAnywhere, Category = "Tile|Debug")
	TObjectPtr<UTextRenderComponent> DebugText;

	// Debug: green outline along the tile edges.
	UPROPERTY(VisibleAnywhere, Category = "Tile|Debug")
	TObjectPtr<UInstancedStaticMeshComponent> DebugBorder;

	UPROPERTY(EditDefaultsOnly, Category = "Tile|Debug")
	float DebugBorderWidth = 40.0f;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Returns the pooled mesh component for the given group index, creating it if needed.
	UInstancedStaticMeshComponent* GetMeshComponent(int32 Index);

	void DestroySpawnedActors();

	// Adds random mineral asteroids to free spots (server only). Needs meshes and actors placed first.
	void SpawnMineralAsteroids(float InTileSize, const UEndlessTileData* Data, int32 RotationSteps,
		const FMineralAsteroidSpawnSettings& Settings, int32 Seed);

	// Weighted by distance of this tile to the base. Null if no sizes are set (class defaults are used).
	const FMineralAsteroidSize* PickAsteroidSize(const FMineralAsteroidSpawnSettings& Settings, FRandomStream& Stream) const;

	void UpdateDebug(float InTileSize, const UEndlessTileData* Data, int32 RotationSteps, bool bShowDebug);

	FIntPoint Coord = FIntPoint::ZeroValue;

	// One instanced component per mesh group, kept across recycles.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> MeshComponents;

	// Gameplay actors from the tile data (server only). Destroyed when the tile is recycled.
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> SpawnedActors;

	// BasicShapeMaterial, tinted green for the debug outline.
	UPROPERTY()
	TObjectPtr<UMaterialInterface> DebugBaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DebugBorderMaterial;
};
