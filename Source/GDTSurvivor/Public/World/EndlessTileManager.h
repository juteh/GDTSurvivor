#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/EndlessTile.h"
#include "EndlessTileManager.generated.h"

class AEndlessTile;
class ANavMeshBoundsVolume;
class UEndlessTileData;

/**
 * Keeps a (2 * GridRadius + 1)^2 grid of AEndlessTile around every player pawn and
 * recycles tiles that fall out of range, so the map feels infinite (Vampire Survivors style).
 * Tile content comes from designer-made tile maps, baked into UEndlessTileData assets.
 * Tiles are not replicated: each machine builds the same layout locally from Seed.
 * The server rolls a new Seed on every level start and replicates it, so clients
 * rebuild their tiles once it arrives.
 * On the server the grid covers all players, on clients only the local one.
 *
 * Place exactly one instance in the level.
 */
UCLASS()
class GDTSURVIVOR_API AEndlessTileManager : public AActor
{
	GENERATED_BODY()

public:
	AEndlessTileManager();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Endless|Tiles")
	TSubclassOf<AEndlessTile> TileClass;

	// Edge length of one tile in unreal units.
	UPROPERTY(EditAnywhere, Category = "Endless|Tiles", meta = (ClampMin = "1000"))
	float TileSize = 5000.0f;

	// 1 = 3x3 tiles around the player, 2 = 5x5, ...
	UPROPERTY(EditAnywhere, Category = "Endless|Tiles", meta = (ClampMin = "1"))
	int32 GridRadius = 2;

	// Off: every level start rolls a new Seed (written to the log). On: always use Seed,
	// e.g. to reproduce a layout from the log while debugging.
	UPROPERTY(EditAnywhere, Category = "Endless|Tiles")
	bool bUseFixedSeed = false;

	// Same seed = same map: tile choice, tile rotation and mineral asteroids.
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Seed, Category = "Endless|Tiles", meta = (EditCondition = "bUseFixedSeed"))
	int32 Seed = 1337;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Always placed at tile (0, 0), which is centered on the world origin. Never rotated.
	UPROPERTY(EditAnywhere, Category = "Endless|Tiles")
	TObjectPtr<UEndlessTileData> StartTile;

	// Pool for all other tiles, picked randomly by their Weight.
	UPROPERTY(EditAnywhere, Category = "Endless|Tiles")
	TArray<TObjectPtr<UEndlessTileData>> TileSet;

	// How free spots for random mineral asteroids are found. What spawns where is set in the
	// Mineral Catalog (Project Settings) and per tile (AsteroidPlacement in the tile maps).
	UPROPERTY(EditAnywhere, Category = "Endless|MineralAsteroids")
	FMineralAsteroidPlacementSettings AsteroidPlacement;

	// Shows tile name, coordinate, rotation and a green outline on every tile.
	UPROPERTY(EditAnywhere, Category = "Endless|Debug")
	bool bShowTileDebug = true;

	// Background plane that follows the local player every frame. Needs a material with
	// world-position based UVs (M_PixelStarfield_World), so the stars stay fixed in the world
	// while the plane itself moves.
	UPROPERTY(EditInstanceOnly, Category = "Endless|Background")
	TObjectPtr<AActor> BackgroundActor;

	// Bounds volume that is moved along with the local player (snapped to the tile grid),
	// so the NavMesh only exists around the player. The level's RecastNavMesh must use
	// Runtime Generation = Dynamic for this.
	UPROPERTY(EditInstanceOnly, Category = "Endless|Navigation")
	TObjectPtr<ANavMeshBoundsVolume> NavBoundsVolume;

	// NavMesh covers (2 * NavGridRadius + 1)^2 tiles around the player.
	// Keep it <= GridRadius, there is no floor to build on outside the tile grid.
	UPROPERTY(EditAnywhere, Category = "Endless|Navigation", meta = (ClampMin = "1"))
	int32 NavGridRadius = 2;

protected:
	virtual void BeginPlay() override;

private:
	// Clients: the server's Seed arrived after the grid was built, re-init every active tile.
	UFUNCTION()
	void OnRep_Seed();

	FIntPoint WorldToTile(const FVector& WorldLocation) const;

	// Collects the tile coordinates needed around all relevant player pawns.
	void GatherRequiredTiles(TSet<FIntPoint>& OutRequired) const;

	void AddTilesAround(const FIntPoint& Center, TSet<FIntPoint>& OutRequired) const;

	void RefreshTiles(const TSet<FIntPoint>& Required);

	// Deterministic per coordinate + Seed. Returns null if the set is empty.
	const UEndlessTileData* PickTile(const FIntPoint& TileCoord, int32& OutRotationSteps) const;

	void UpdateBackground();

	// Re-centers NavBoundsVolume on the local player's tile when it changed.
	void UpdateNavBounds();

	UPROPERTY(Transient)
	TMap<FIntPoint, TObjectPtr<AEndlessTile>> ActiveTiles;

	// Height of the background plane, kept while following the player.
	double BackgroundZ = 0.0;

	// Pawn tiles from the last refresh, to skip work while nobody changes tiles.
	TSet<FIntPoint> LastPawnTiles;

	TOptional<FIntPoint> NavBoundsCenter;
};
