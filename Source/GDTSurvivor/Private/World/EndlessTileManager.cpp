#include "World/EndlessTileManager.h"

#include "World/EndlessTile.h"
#include "World/EndlessTileData.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

AEndlessTileManager::AEndlessTileManager()
{
	// Every frame: the background has to follow the player smoothly.
	// The tile check itself is cheap (one coordinate per player).
	PrimaryActorTick.bCanEverTick = true;
	// Run after the pawns moved, so the background never lags one frame behind.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	TileClass = AEndlessTile::StaticClass();
}

void AEndlessTileManager::BeginPlay()
{
	Super::BeginPlay();

	if (BackgroundActor && BackgroundActor->GetRootComponent())
	{
		BackgroundZ = BackgroundActor->GetActorLocation().Z;
		// Placed level meshes are Static by default and refuse to move at runtime.
		BackgroundActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		// Purely visual; a 70000uu collider teleported every frame would only cost physics time.
		BackgroundActor->SetActorEnableCollision(false);
	}

	if (NavBoundsVolume && NavBoundsVolume->GetRootComponent())
	{
		NavBoundsVolume->GetRootComponent()->SetMobility(EComponentMobility::Movable);

		// Resize the volume in XY to exactly cover the nav tile area; Z stays as placed.
		const FVector CurrentSize = NavBoundsVolume->GetComponentsBoundingBox(true).GetSize();
		if (CurrentSize.X > 0.0 && CurrentSize.Y > 0.0)
		{
			const double DesiredSize = (2 * NavGridRadius + 1) * TileSize;
			FVector Scale = NavBoundsVolume->GetActorScale3D();
			Scale.X *= DesiredSize / CurrentSize.X;
			Scale.Y *= DesiredSize / CurrentSize.Y;
			NavBoundsVolume->SetActorScale3D(Scale);
		}
	}

	// Build the grid right away, so the player never spawns into an empty world.
	// Pawns may not exist yet at this point, then the start tile is used as center.
	TSet<FIntPoint> Required;
	GatherRequiredTiles(Required);
	if (Required.IsEmpty())
	{
		AddTilesAround(FIntPoint::ZeroValue, Required);
	}
	RefreshTiles(Required);
	UpdateNavBounds();
}

void AEndlessTileManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TSet<FIntPoint> PawnTiles;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			PawnTiles.Add(WorldToTile(PC->GetPawn()->GetActorLocation()));
		}
	}

	if (PawnTiles.Num() > 0 && !PawnTiles.Difference(LastPawnTiles).IsEmpty())
	{
		TSet<FIntPoint> Required;
		GatherRequiredTiles(Required);
		RefreshTiles(Required);
	}
	LastPawnTiles = MoveTemp(PawnTiles);

	UpdateNavBounds();
	UpdateBackground();
}

FIntPoint AEndlessTileManager::WorldToTile(const FVector& WorldLocation) const
{
	// Tiles are centered on multiples of TileSize, so tile (0, 0) is centered on the world origin.
	return FIntPoint(
		FMath::FloorToInt32(WorldLocation.X / TileSize + 0.5),
		FMath::FloorToInt32(WorldLocation.Y / TileSize + 0.5));
}

const UEndlessTileData* AEndlessTileManager::PickTile(const FIntPoint& TileCoord, int32& OutRotationSteps) const
{
	OutRotationSteps = 0;

	if (TileCoord == FIntPoint::ZeroValue && StartTile)
	{
		return StartTile;
	}

	float TotalWeight = 0.0f;
	for (const UEndlessTileData* Candidate : TileSet)
	{
		if (Candidate)
		{
			TotalWeight += FMath::Max(Candidate->Weight, 0.0f);
		}
	}
	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	FRandomStream Stream(HashCombine(GetTypeHash(TileCoord), GetTypeHash(Seed)));
	float Roll = Stream.FRandRange(0.0f, TotalWeight);
	const UEndlessTileData* Picked = nullptr;
	for (const UEndlessTileData* Candidate : TileSet)
	{
		if (!Candidate || Candidate->Weight <= 0.0f)
		{
			continue;
		}
		Picked = Candidate;
		Roll -= Candidate->Weight;
		if (Roll <= 0.0f)
		{
			break;
		}
	}

	if (Picked && Picked->bAllowRotation)
	{
		OutRotationSteps = Stream.RandRange(0, 3);
	}
	return Picked;
}

void AEndlessTileManager::GatherRequiredTiles(TSet<FIntPoint>& OutRequired) const
{
	// Server: all players. Client: only its local player controller exists.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			AddTilesAround(WorldToTile(PC->GetPawn()->GetActorLocation()), OutRequired);
		}
	}
}

void AEndlessTileManager::AddTilesAround(const FIntPoint& Center, TSet<FIntPoint>& OutRequired) const
{
	for (int32 X = -GridRadius; X <= GridRadius; ++X)
	{
		for (int32 Y = -GridRadius; Y <= GridRadius; ++Y)
		{
			OutRequired.Add(Center + FIntPoint(X, Y));
		}
	}
}

void AEndlessTileManager::RefreshTiles(const TSet<FIntPoint>& Required)
{
	if (Required.IsEmpty() || !TileClass)
	{
		return;
	}

	// Tiles that are no longer needed become free for reuse.
	TArray<AEndlessTile*> FreeTiles;
	for (auto It = ActiveTiles.CreateIterator(); It; ++It)
	{
		if (!Required.Contains(It.Key()))
		{
			if (It.Value())
			{
				FreeTiles.Add(It.Value());
			}
			It.RemoveCurrent();
		}
	}

	for (const FIntPoint& TileCoord : Required)
	{
		if (ActiveTiles.Contains(TileCoord))
		{
			continue;
		}

		AEndlessTile* Tile = FreeTiles.Num() > 0 ? FreeTiles.Pop(EAllowShrinking::No) : nullptr;
		if (!Tile)
		{
			FActorSpawnParameters Params;
			Params.Owner = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Tile = GetWorld()->SpawnActor<AEndlessTile>(TileClass, FTransform::Identity, Params);
		}

		if (Tile)
		{
			int32 RotationSteps = 0;
			const UEndlessTileData* Data = PickTile(TileCoord, RotationSteps);
			Tile->InitTile(TileCoord, TileSize, Data, RotationSteps, MineralAsteroids, Seed, bShowTileDebug);
			ActiveTiles.Add(TileCoord, Tile);
		}
	}

	// Leftovers only happen when the required area shrinks (e.g. a player left).
	for (AEndlessTile* Tile : FreeTiles)
	{
		Tile->Destroy();
	}
}

void AEndlessTileManager::UpdateNavBounds()
{
	if (!NavBoundsVolume)
	{
		return;
	}

	// One volume only, so it follows the first local player (multiplayer would need one per player).
	const APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	const APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;
	const FIntPoint Center = LocalPawn ? WorldToTile(LocalPawn->GetActorLocation()) : FIntPoint::ZeroValue;

	if (NavBoundsCenter.IsSet() && NavBoundsCenter.GetValue() == Center)
	{
		return;
	}
	NavBoundsCenter = Center;

	const FVector NewLocation(Center.X * TileSize, Center.Y * TileSize, NavBoundsVolume->GetActorLocation().Z);
	NavBoundsVolume->SetActorLocation(NewLocation);

	// Only the tiles that entered the bounds get built (Dynamic runtime generation).
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->OnNavigationBoundsUpdated(NavBoundsVolume);
	}
}

void AEndlessTileManager::UpdateBackground()
{
	if (!BackgroundActor)
	{
		return;
	}

	const APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	const APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;
	if (!LocalPawn)
	{
		return;
	}

	const FVector PawnLocation = LocalPawn->GetActorLocation();
	const FVector NewLocation(PawnLocation.X, PawnLocation.Y, BackgroundZ);
	if (NewLocation.Equals(BackgroundActor->GetActorLocation()))
	{
		return;
	}

	BackgroundActor->SetActorLocation(NewLocation);

	// The stars are world-aligned in the material, so on screen they don't move with the
	// plane. Without this, the plane's own motion would be fed into motion blur / TSR
	// and smear the stars.
	BackgroundActor->ForEachComponent<UPrimitiveComponent>(false, [](UPrimitiveComponent* Primitive)
	{
		Primitive->ResetSceneVelocity();
	});
}
