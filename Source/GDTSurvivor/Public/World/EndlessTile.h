#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minerals/MineralCatalog.h"
#include "EndlessTile.generated.h"

class AMineralAsteroid;
class UEndlessTileData;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMineralDefinition;
class UStaticMeshComponent;
class UTextRenderComponent;

// Technical rules for finding free spots for random mineral asteroids (set on the EndlessTileManager).
// Which mineral and size, and how many per tile, is game design: see the Mineral Catalog and the tile maps.
USTRUCT(BlueprintType)
struct FMineralAsteroidPlacementSettings
{
	GENERATED_BODY()

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
	// Mineral asteroids are deterministic per coordinate and Seed.
	void InitTile(const FIntPoint& InCoord, float InTileSize, const UEndlessTileData* Data,
		int32 RotationSteps, const FMineralAsteroidPlacementSettings& AsteroidPlacement, int32 Seed, bool bShowDebug);

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

	// Adds mineral asteroids at the tile's markers and/or on random free spots, as set in the tile
	// data; mineral and size come from the zone of this tile (server only). Needs meshes and actors
	// placed first.
	void SpawnMineralAsteroids(float InTileSize, const UEndlessTileData* Data, int32 RotationSteps,
		const FMineralAsteroidPlacementSettings& Settings, int32 Seed);

	// Spawns one asteroid at the tile-local XY location. Returns its footprint radius, or 0 if nothing spawned.
	float SpawnMineralAsteroid(const UMineralCatalog& Catalog, const UMineralDefinition& Mineral,
		EMineralAsteroidSize Size, const FVector2D& Local);

	// Footprint radius of the mineral's asteroid at the given size.
	static float GetAsteroidRadius(const UMineralCatalog& Catalog, const UMineralDefinition& Mineral, EMineralAsteroidSize Size);

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
