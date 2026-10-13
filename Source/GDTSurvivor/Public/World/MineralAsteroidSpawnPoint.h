#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minerals/MineralCatalog.h"
#include "MineralAsteroidSpawnPoint.generated.h"

class UMineralDefinition;
class USphereComponent;

/**
 * Marks a spot for a mineral asteroid in a tile map. "Bake" on the EndlessTileAuthoring turns it
 * into an asteroid slot; it is only used if the tile's AsteroidPlacement includes Markers.
 * The marker always wins over the zone: the size is picked only from MinSize..MaxSize (equal chance),
 * the zone of the tile only picks the mineral, unless FixedMineral is set.
 *
 * The cyan circle shows the biggest asteroid that can appear here. Editor only.
 */
UCLASS(HideCategories = (Collision, Physics, Input, HLOD, Replication, Networking, Cooking, DataLayers, WorldPartition, LevelInstance, Rendering))
class GDTSURVIVOR_API AMineralAsteroidSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AMineralAsteroidSpawnPoint();

	// Chance that an asteroid appears here at all (1 = always).
	UPROPERTY(EditAnywhere, Category = "Asteroid", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Asteroid")
	EMineralAsteroidSize MinSize = EMineralAsteroidSize::Small;

	UPROPERTY(EditAnywhere, Category = "Asteroid")
	EMineralAsteroidSize MaxSize = EMineralAsteroidSize::Huge;

	// Always this mineral, regardless of the zone (e.g. a guaranteed Iconos hidden in a wreck).
	// Empty = picked by the zone.
	UPROPERTY(EditAnywhere, Category = "Asteroid")
	TObjectPtr<UMineralDefinition> FixedMineral;

	virtual bool IsEditorOnly() const override { return true; }

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category = "Asteroid")
	TObjectPtr<USphereComponent> MaxSizePreview;
};
