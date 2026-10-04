#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EndlessTileData.generated.h"

class UMaterialInterface;
class UStaticMesh;

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

	// Random mineral asteroids may be added to free spots of this tile.
	UPROPERTY(VisibleAnywhere, Category = "Tile")
	bool bAllowMineralAsteroids = true;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TSoftObjectPtr<UWorld> SourceMap;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TArray<FEndlessTileMeshGroup> MeshGroups;

	UPROPERTY(VisibleAnywhere, Category = "Tile|Baked")
	TArray<FEndlessTileActorEntry> Actors;
};
