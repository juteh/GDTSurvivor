#include "Minerals/MineralDefinition.h"

#include "World/MineralAsteroid.h"

TSubclassOf<AActor> UMineralDefinition::GetMineralClass() const
{
	return AsteroidClass ? AsteroidClass->GetDefaultObject<AMineralAsteroid>()->MineralClass : nullptr;
}
