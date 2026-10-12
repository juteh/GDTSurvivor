#include "World/MineralAsteroidSpawnPoint.h"

#include "Components/SphereComponent.h"
#include "Minerals/MineralDefinition.h"
#include "World/MineralAsteroid.h"

AMineralAsteroidSpawnPoint::AMineralAsteroidSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	MaxSizePreview = CreateDefaultSubobject<USphereComponent>(TEXT("MaxSizePreview"));
	MaxSizePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaxSizePreview->SetCanEverAffectNavigation(false);
	MaxSizePreview->ShapeColor = FColor::Cyan;
	MaxSizePreview->SetLineThickness(15.0f);
	RootComponent = MaxSizePreview;

	SetActorHiddenInGame(true);
}

#if WITH_EDITOR

void AMineralAsteroidSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (MaxSize < MinSize)
	{
		MaxSize = MinSize;
	}

	// Footprint of the biggest allowed size; asteroid collision is authored for scale 1.
	float Radius = 50.0f;
	float Scale = 1.0f;
	if (const UMineralCatalog* Catalog = UMineralCatalog::Get())
	{
		if (const FMineralAsteroidSizeStats* Stats = Catalog->Sizes.Find(MaxSize))
		{
			Scale = Stats->Scale;
		}
		const UMineralDefinition* Mineral = FixedMineral ? FixedMineral.Get()
			: (Catalog->Minerals.Num() > 0 ? Catalog->Minerals[0].Get() : nullptr);
		if (Mineral && Mineral->AsteroidClass)
		{
			Radius = Mineral->AsteroidClass->GetDefaultObject<AMineralAsteroid>()->GetFootprintRadius();
		}
	}
	// The actor's own scale is ignored by the bake, so the circle ignores it too.
	MaxSizePreview->SetSphereRadius(Radius * Scale / FMath::Max(GetActorScale3D().GetAbsMax(), KINDA_SMALL_NUMBER));
}

#endif
