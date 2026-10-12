#include "World/EndlessTileData.h"

void UEndlessTileData::PostLoad()
{
	Super::PostLoad();

	// Assets baked before AsteroidPlacement existed.
	if (!bAllowMineralAsteroids)
	{
		AsteroidPlacement = EMineralAsteroidPlacement::None;
		bAllowMineralAsteroids = true;
	}
}
