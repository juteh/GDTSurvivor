#include "EndlessTile.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EndlessTileData.h"
#include "MineralAsteroid.h"
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

	// Debug visuals sit above the usual 700 high blocks (centered on Z = 0).
	constexpr float DebugHeight = 400.0f;
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
	DebugText->SetRelativeLocation(FVector(0.0, 0.0, DebugHeight));
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
	int32 RotationSteps, const FMineralAsteroidSpawnSettings& AsteroidSettings, int32 Seed, bool bShowDebug)
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

		if (Data->bAllowMineralAsteroids)
		{
			SpawnMineralAsteroids(InTileSize, Data, RotationSteps, AsteroidSettings, Seed);
		}
	}

	UpdateDebug(InTileSize, Data, RotationSteps, bShowDebug);
}

void AEndlessTile::SpawnMineralAsteroids(float InTileSize, const UEndlessTileData* Data, int32 RotationSteps,
	const FMineralAsteroidSpawnSettings& Settings, int32 Seed)
{
	if (Settings.Classes.IsEmpty())
	{
		return;
	}

	// Separate stream from the tile pick, so changing asteroid settings doesn't reshuffle the tiles.
	constexpr uint32 AsteroidSalt = 0xA57E401Du;
	FRandomStream Stream(HashCombine(HashCombine(GetTypeHash(Coord), GetTypeHash(Seed)), AsteroidSalt));

	int32 NumAsteroids = 0;
	while (NumAsteroids < Settings.MaxPerTile && Stream.FRand() < Settings.Chance)
	{
		++NumAsteroids;
	}
	if (NumAsteroids == 0)
	{
		return;
	}

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

	const float Half = 0.5f * InTileSize;
	for (int32 AsteroidIndex = 0; AsteroidIndex < NumAsteroids; ++AsteroidIndex)
	{
		const TSubclassOf<AMineralAsteroid> AsteroidClass = Settings.Classes[Stream.RandRange(0, Settings.Classes.Num() - 1)];
		if (!AsteroidClass)
		{
			continue;
		}
		const FMineralAsteroidSize* Size = PickAsteroidSize(Settings, Stream);
		// Without size table: biggest size, matching the class defaults (MaxHealth / MineralCount).
		const float Scale = Size ? Size->Scale : 8.0f;
		// Class default radius is for scale 1.
		const float Radius = AsteroidClass->GetDefaultObject<AMineralAsteroid>()->GetFootprintRadius() * Scale;
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

			const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(TileCenter + Local, 0.0), FVector(Scale));
			if (AMineralAsteroid* Asteroid = GetWorld()->SpawnActorDeferred<AMineralAsteroid>(AsteroidClass, SpawnTransform,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				if (Size)
				{
					Asteroid->SetSizeStats(Size->Health, Size->Minerals);
				}
				Asteroid->FinishSpawning(SpawnTransform);
				SpawnedActors.Add(Asteroid);
				Occupied.Emplace(Local, Radius);
			}
			break;
		}
	}
}

const FMineralAsteroidSize* AEndlessTile::PickAsteroidSize(const FMineralAsteroidSpawnSettings& Settings, FRandomStream& Stream) const
{
	// 0 at the base tile, 1 at FarDistanceInTiles and beyond.
	const float Alpha = FMath::Clamp(FVector2D(Coord).Size() / Settings.FarDistanceInTiles, 0.0f, 1.0f);

	float TotalWeight = 0.0f;
	for (const FMineralAsteroidSize& Size : Settings.Sizes)
	{
		TotalWeight += FMath::Max(FMath::Lerp(Size.NearWeight, Size.FarWeight, Alpha), 0.0f);
	}
	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Roll = Stream.FRandRange(0.0f, TotalWeight);
	for (const FMineralAsteroidSize& Size : Settings.Sizes)
	{
		Roll -= FMath::Max(FMath::Lerp(Size.NearWeight, Size.FarWeight, Alpha), 0.0f);
		if (Roll <= 0.0f)
		{
			return &Size;
		}
	}
	return &Settings.Sizes.Last();
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

	const FString Name = Data ? Data->TileName : TEXT("(leer)");
	DebugText->SetText(FText::FromString(FString::Printf(TEXT("%s<br>%d, %d %d"),
		*Name, Coord.X, Coord.Y, RotationSteps * 90)));

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
