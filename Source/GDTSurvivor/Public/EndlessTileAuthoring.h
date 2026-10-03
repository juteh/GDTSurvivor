#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EndlessTileAuthoring.generated.h"

class UBoxComponent;
class UEndlessTileData;

/**
 * Helper for building tiles as maps. Place exactly one at the origin of a tile map
 * (Tile_A, Tile_B, ...), build the tile around it and press "Bake".
 *
 * Bake collects everything in the map into a UEndlessTileData asset:
 *  - StaticMeshActors become instanced meshes (blocks, walls, decoration)
 *  - Blueprint actors (minerals, pickups, ...) are spawned at runtime
 *  - native engine actors (lights, volumes, PlayerStart, ...) and actors tagged "NoBake" are ignored
 *
 * Editor only, never part of the game.
 */
UCLASS(HideCategories = (Collision, Physics, Input, HLOD, Replication, Networking, Cooking, DataLayers, WorldPartition, LevelInstance))
class GDTSURVIVOR_API AEndlessTileAuthoring : public AActor
{
	GENERATED_BODY()

public:
	AEndlessTileAuthoring();

	// Shown on the tile in debug mode. Empty = map name.
	UPROPERTY(EditAnywhere, Category = "Tile")
	FString TileName;

	// May be placed rotated by 90/180/270 degrees.
	UPROPERTY(EditAnywhere, Category = "Tile")
	bool bAllowRotation = true;

	// Relative chance to be picked compared to the other tiles in the set.
	UPROPERTY(EditAnywhere, Category = "Tile", meta = (ClampMin = "0"))
	float Weight = 1.0f;

	// Random mineral asteroids may be added to free spots of this tile (settings on the EndlessTileManager).
	UPROPERTY(EditAnywhere, Category = "Tile")
	bool bAllowMineralAsteroids = true;

	// Must match TileSize on the EndlessTileManager.
	UPROPERTY(EditAnywhere, Category = "Tile", meta = (ClampMin = "1000"))
	float TileSize = 5000.0f;

	// Width of the border strip that should stay free, so random neighbours never block each other.
	// Only a visual guide (orange box), not enforced.
	UPROPERTY(EditAnywhere, Category = "Tile", meta = (ClampMin = "0"))
	float EdgeMargin = 500.0f;

	// Asset the bake writes into. Created next to the map as DA_<MapName> if empty.
	UPROPERTY(EditAnywhere, Category = "Tile")
	TObjectPtr<UEndlessTileData> TargetData;

	virtual bool IsEditorOnly() const override { return true; }

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Tile")
	void Bake();

	virtual void OnConstruction(const FTransform& Transform) override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TObjectPtr<UBoxComponent> TileBounds;

	UPROPERTY(VisibleAnywhere, Category = "Tile")
	TObjectPtr<UBoxComponent> FreeEdgeBounds;

#if WITH_EDITOR
	UEndlessTileData* FindOrCreateDataAsset();
#endif
};
