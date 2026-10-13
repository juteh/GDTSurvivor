#include "World/EndlessTile.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Minerals/MineralCatalog.h"
#include "Minerals/MineralDefinition.h"
#include "World/EndlessTileData.h"
#include "World/MineralAsteroid.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Size of the engine basic shapes (Cube / Plane) in unreal units.
	constexpr float BasicShapeSize = 100.0f;

	// Debug visuals sit below the usual 700 high blocks (centered on Z = 0), so ships, pickups and walls
	// are drawn over them; the background plane is much lower.
	constexpr float DebugHeight = -400.0f;
}

AEndlessTile::AEndlessTile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Tiles are moved around at runtime, so everything has to be Movable.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	NavFloor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NavFloor"));
	NavFloor->SetupAttachment(Root);
	NavFloor->SetMobility(EComponentMobility::Movable);
	NavFloor->SetStaticMesh(PlaneMesh.Object);
	NavFloor->SetVisibility(false);
	NavFloor->SetCastShadow(false);
	NavFloor->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	NavFloor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// Not WorldStatic on purpose: BP_WaveTrigger's spawn-occupancy trace queries
	// WorldStatic/WorldDynamic and would otherwise always hit the floor.
	// Vehicle is unused in this project; NavMesh generation ignores the object type.
	NavFloor->SetCollisionObjectType(ECC_Vehicle);
	NavFloor->SetCollisionResponseToAllChannels(ECR_Ignore);
	NavFloor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	NavFloor->SetCanEverAffectNavigation(true);

	DebugBaseMaterial = BasicShapeMaterial.Object;

	DebugText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugText"));
	DebugText->SetupAttachment(Root);
	DebugText->SetMobility(EComponentMobility::Movable);
	// Lying flat, readable from the player camera (looks along +X, so screen-up is +X).
	// Height is set in UpdateDebug.
	DebugText->SetRelativeRotation(FRotator(90.0, 180.0, 0.0));
	DebugText->SetHorizontalAlignment(EHTA_Center);
	DebugText->SetVerticalAlignment(EVRTA_TextCenter);
	DebugText->SetWorldSize(500.0f);
	DebugText->SetTextRenderColor(FColor::Green);
	DebugText->SetText(FText::FromString(TEXT("0, 0")));
	DebugText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugText->SetCanEverAffectNavigation(false);
	DebugText->SetCastShadow(false);

	DebugBorder = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DebugBorder"));
	DebugBorder->SetupAttachment(Root);
	DebugBorder->SetMobility(EComponentMobility::Movable);
	DebugBorder->SetStaticMesh(CubeMesh.Object);
	DebugBorder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugBorder->SetCanEverAffectNavigation(false);
	DebugBorder->SetCastShadow(false);

	// Projectiles / HandleProjectileHit treat actors tagged "level" as walls.
	Tags.Add(FName("level"));
}

void AEndlessTile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroySpawnedActors();
	Super::EndPlay(EndPlayReason);
}

void AEndlessTile::InitTile(const FIntPoint& InCoord, float InTileSize, const UEndlessTileData* Data,
	int32 RotationSteps, const FMineralAsteroidPlacementSettings& AsteroidPlacement, int32 Seed, bool bShowDebug)
{
	Coord = InCoord;

	const FVector TileCenter(Coord.X * InTileSize, Coord.Y * InTileSize, 0.0);
	SetActorLocation(TileCenter, false, nullptr, ETeleportType::TeleportPhysics);

	NavFloor->SetRelativeScale3D(FVector(InTileSize / BasicShapeSize, InTileSize / BasicShapeSize, 1.0));

	DestroySpawnedActors();

	// Rotation around the tile center, applied after each baked relative transform.
	const FTransform TileRotation(FRotator(0.0, 90.0 * RotationSteps, 0.0));

	const int32 NumGroups = Data ? Data->MeshGroups.Num() : 0;
	for (int32 GroupIndex = 0; GroupIndex < NumGroups; ++GroupIndex)
	{
		const FEndlessTileMeshGroup& Group = Data->MeshGroups[GroupIndex];
		UInstancedStaticMeshComponent* Meshes = GetMeshComponent(GroupIndex);

		Meshes->ClearInstances();
		Meshes->SetStaticMesh(Group.Mesh);
		for (int32 MaterialIndex = 0; MaterialIndex < Group.Materials.Num(); ++MaterialIndex)
		{
			Meshes->SetMaterial(MaterialIndex, Group.Materials[MaterialIndex]);
		}
		Meshes->SetCollisionProfileName(Group.CollisionProfile);

		TArray<FTransform> Instances;
		Instances.Reserve(Group.Instances.Num());
		for (const FTransform& Instance : Group.Instances)
		{
			Instances.Add(Instance * TileRotation);
		}
		Meshes->AddInstances(Instances, false);
		Meshes->ResetSceneVelocity();
	}

	// Pooled components the new data doesn't need.
	for (int32 Index = NumGroups; Index < MeshComponents.Num(); ++Index)
	{
		MeshComponents[Index]->ClearInstances();
	}

	// Gameplay actors are server-side; they replicate themselves if their class does.
	if (Data && HasAuthority())
	{
		const FTransform TileTransform(TileCenter);

		for (const FEndlessTileActorEntry& Entry : Data->Actors)
		{
			if (!Entry.ActorClass)
			{
				continue;
			}
			const FTransform SpawnTransform = Entry.Transform * TileRotation * TileTransform;

			// Deferred on purpose: plain SpawnActor finishes with bIsDefaultTransform = true, and then
			// Blueprints with an SCS root (DefaultSceneRoot) get the template's scale instead of ours
			// (SCS_Node.cpp). FinishSpawning with an explicit transform keeps the baked scale.
			AActor* Spawned = GetWorld()->SpawnActorDeferred<AActor>(Entry.ActorClass, SpawnTransform,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Spawned)
			{
				Spawned->FinishSpawning(SpawnTransform);
				SpawnedActors.Add(Spawned);
			}
		}

		if (Data->AsteroidPlacement != EMineralAsteroidPlacement::None)
		{
			SpawnMineralAsteroids(InTileSize, Data, RotationSteps, AsteroidPlacement, Seed);
		}
	}

	UpdateDebug(InTileSize, Data, RotationSteps, bShowDebug);
}

void AEndlessTile::SpawnMineralAsteroids(float InTileSize, const UEndlessTileData* Data, int32 RotationSteps,
	const FMineralAsteroidPlacementSettings& Settings, int32 Seed)
{
	const UMineralCatalog* Catalog = UMineralCatalog::Get();
	if (!Catalog)
	{
		return;
	}
	// May be null (no zones); then only markers with a FixedMineral spawn.
	const FMineralZone* Zone = Catalog->FindZone(Coord);

	// Separate stream from the tile pick, so changing asteroid settings doesn't reshuffle the tiles.
	constexpr uint32 AsteroidSalt = 0xA57E401Du;
	FRandomStream Stream(HashCombine(HashCombine(GetTypeHash(Coord), GetTypeHash(Seed)), AsteroidSalt));

	// Everything already in the tile as 2D circles (tile-local center, radius).
	TArray<TPair<FVector2D, float>> Occupied;
	const FTransform TileRotation(FRotator(0.0, 90.0 * RotationSteps, 0.0));
	for (const FEndlessTileMeshGroup& Group : Data->MeshGroups)
	{
		if (!Group.Mesh)
		{
			continue;
		}
		const FBoxSphereBounds MeshBounds = Group.Mesh->GetBounds();
		for (const FTransform& Instance : Group.Instances)
		{
			const FTransform Placed = Instance * TileRotation;
			const FVector Extent = MeshBounds.BoxExtent * Placed.GetScale3D().GetAbs();
			Occupied.Emplace(FVector2D(Placed.TransformPosition(MeshBounds.Origin)), FVector2D(Extent).Size());
		}
	}
	const FVector2D TileCenter(GetActorLocation());
	for (const TWeakObjectPtr<AActor>& Spawned : SpawnedActors)
	{
		if (Spawned.IsValid())
		{
			FVector Origin, Extent;
			Spawned->GetActorBounds(true, Origin, Extent);
			Occupied.Emplace(FVector2D(Origin) - TileCenter, FVector2D(Extent).Size());
		}
	}

	const bool bUseMarkers = Data->AsteroidPlacement == EMineralAsteroidPlacement::Markers
		|| Data->AsteroidPlacement == EMineralAsteroidPlacement::MarkersAndRandom;
	const bool bUseRandom = Data->AsteroidPlacement == EMineralAsteroidPlacement::Random
		|| Data->AsteroidPlacement == EMineralAsteroidPlacement::MarkersAndRandom;

	// Markers: placed by the designer, so they always win: no free-spot check, and the size comes only
	// from the marker's range (the zone only picks the mineral, unless FixedMineral is set).
	// They block the random ones.
	if (bUseMarkers)
	{
		for (const FEndlessTileAsteroidSlot& Slot : Data->AsteroidSlots)
		{
			if (Stream.FRand() >= Slot.Chance)
			{
				continue;
			}
			EMineralAsteroidSize Size = EMineralAsteroidSize::Small;
			const UMineralDefinition* Mineral = Slot.FixedMineral ? Slot.FixedMineral.Get()
				: Zone ? Catalog->PickMineral(*Zone, Stream) : nullptr;
			if (!Mineral || !Catalog->PickSizeInRange(Slot.MinSize, Slot.MaxSize, Stream, Size))
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: asteroid marker at %s skipped (no mineral in the zone or no size in %d..%d)."),
					*Data->TileName, *Slot.Location.ToString(), static_cast<int32>(Slot.MinSize), static_cast<int32>(Slot.MaxSize));
				continue;
			}
			const FVector2D Local(TileRotation.TransformPosition(Slot.Location));
			const float Radius = SpawnMineralAsteroid(*Catalog, *Mineral, Size, Local);
			if (Radius > 0.0f)
			{
				Occupied.Emplace(Local, Radius);
			}
		}
	}

	if (!bUseRandom || !Zone)
	{
		return;
	}

	const int32 NumRandom = Stream.RandRange(Data->MinRandomAsteroids, FMath::Max(Data->MinRandomAsteroids, Data->MaxRandomAsteroids));
	const float Half = 0.5f * InTileSize;
	for (int32 AsteroidIndex = 0; AsteroidIndex < NumRandom; ++AsteroidIndex)
	{
		EMineralAsteroidSize Size = EMineralAsteroidSize::Small;
		const UMineralDefinition* Mineral = Catalog->PickMineral(*Zone, Stream);
		if (!Mineral || !Catalog->PickSize(*Zone, EMineralAsteroidSize::Small, EMineralAsteroidSize::Huge, Stream, Size))
		{
			continue;
		}
		const float Radius = GetAsteroidRadius(*Catalog, *Mineral, Size);
		const float Range = Half - Settings.EdgeMargin - Radius;
		if (Range <= 0.0f)
		{
			continue;
		}

		for (int32 Attempt = 0; Attempt < Settings.PlacementAttempts; ++Attempt)
		{
			const FVector2D Local(Stream.FRandRange(-Range, Range), Stream.FRandRange(-Range, Range));
			const bool bBlocked = Occupied.ContainsByPredicate([&](const TPair<FVector2D, float>& Other)
			{
				return FVector2D::Distance(Local, Other.Key) < Radius + Other.Value + Settings.Clearance;
			});
			if (bBlocked)
			{
				continue;
			}

			if (SpawnMineralAsteroid(*Catalog, *Mineral, Size, Local) > 0.0f)
			{
				Occupied.Emplace(Local, Radius);
			}
			break;
		}
	}
}

float AEndlessTile::GetAsteroidRadius(const UMineralCatalog& Catalog, const UMineralDefinition& Mineral, EMineralAsteroidSize Size)
{
	const FMineralAsteroidSizeStats* Stats = Catalog.Sizes.Find(Size);
	if (!Mineral.AsteroidClass || !Stats)
	{
		return 0.0f;
	}
	// Class default radius is for scale 1.
	return Mineral.AsteroidClass->GetDefaultObject<AMineralAsteroid>()->GetFootprintRadius() * Stats->Scale;
}

float AEndlessTile::SpawnMineralAsteroid(const UMineralCatalog& Catalog, const UMineralDefinition& Mineral,
	EMineralAsteroidSize Size, const FVector2D& Local)
{
	const FMineralAsteroidSizeStats* Stats = Catalog.Sizes.Find(Size);
	if (!Mineral.AsteroidClass || !Stats)
	{
		return 0.0f;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(FVector2D(GetActorLocation()) + Local, 0.0), FVector(Stats->Scale));
	AMineralAsteroid* Asteroid = GetWorld()->SpawnActorDeferred<AMineralAsteroid>(Mineral.AsteroidClass, SpawnTransform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Asteroid)
	{
		return 0.0f;
	}
	Asteroid->SetSizeStats(Stats->Health, Stats->Minerals);
	Asteroid->FinishSpawning(SpawnTransform);
	SpawnedActors.Add(Asteroid);
	return GetAsteroidRadius(Catalog, Mineral, Size);
}

UInstancedStaticMeshComponent* AEndlessTile::GetMeshComponent(int32 Index)
{
	while (MeshComponents.Num() <= Index)
	{
		UInstancedStaticMeshComponent* Meshes = NewObject<UInstancedStaticMeshComponent>(this);
		Meshes->SetMobility(EComponentMobility::Movable);
		Meshes->SetupAttachment(RootComponent);
		// BP_Projectile only reacts to overlaps (OverlapAllDynamic).
		Meshes->SetGenerateOverlapEvents(true);
		Meshes->SetCanEverAffectNavigation(true);
		Meshes->RegisterComponent();
		MeshComponents.Add(Meshes);
	}
	return MeshComponents[Index];
}

void AEndlessTile::DestroySpawnedActors()
{
	for (const TWeakObjectPtr<AActor>& Spawned : SpawnedActors)
	{
		// Already gone if it was collected or destroyed in the meantime.
		if (Spawned.IsValid())
		{
			Spawned->Destroy();
		}
	}
	SpawnedActors.Reset();
}

void AEndlessTile::UpdateDebug(float InTileSize, const UEndlessTileData* Data, int32 RotationSteps, bool bShowDebug)
{
	DebugText->SetVisibility(bShowDebug);
	DebugBorder->SetVisibility(bShowDebug);
	DebugBorder->ClearInstances();
	if (!bShowDebug)
	{
		return;
	}

	DebugText->SetRelativeLocation(FVector(0.0, 0.0, DebugHeight));

	const FString Name = Data ? Data->TileName : TEXT("(leer)");
	const UMineralCatalog* Catalog = UMineralCatalog::Get();
	const FMineralZone* Zone = Catalog ? Catalog->FindZone(Coord) : nullptr;
	const FString ZoneName = Zone ? Zone->DisplayName.ToString() : TEXT("-");
	DebugText->SetText(FText::FromString(FString::Printf(TEXT("%s<br>%d, %d %d<br>%s"),
		*Name, Coord.X, Coord.Y, RotationSteps * 90, *ZoneName)));

	if (!DebugBorderMaterial && DebugBaseMaterial)
	{
		// BasicShapeMaterial exposes a "Color" vector parameter.
		DebugBorderMaterial = UMaterialInstanceDynamic::Create(DebugBaseMaterial, this);
		DebugBorderMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
		DebugBorder->SetMaterial(0, DebugBorderMaterial);
	}

	// Four thin flat strips along the inside of the tile edges.
	const float Half = 0.5f * InTileSize;
	const float Inset = Half - 0.5f * DebugBorderWidth;
	const FVector AlongX(InTileSize / BasicShapeSize, DebugBorderWidth / BasicShapeSize, 0.05);
	const FVector AlongY(DebugBorderWidth / BasicShapeSize, InTileSize / BasicShapeSize, 0.05);

	DebugBorder->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0, Inset, DebugHeight), AlongX));
	DebugBorder->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0, -Inset, DebugHeight), AlongX));
	DebugBorder->AddInstance(FTransform(FRotator::ZeroRotator, FVector(Inset, 0.0, DebugHeight), AlongY));
	DebugBorder->AddInstance(FTransform(FRotator::ZeroRotator, FVector(-Inset, 0.0, DebugHeight), AlongY));
	DebugBorder->ResetSceneVelocity();
}
