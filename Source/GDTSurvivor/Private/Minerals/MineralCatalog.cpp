#include "Minerals/MineralCatalog.h"

#include "Game/GDTSurvivorSettings.h"
#include "Minerals/MineralDefinition.h"
#include "World/MineralAsteroid.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogMineralCatalog, Log, All);

namespace
{
	// Weighted pick over the entries of a map that pass the filter.
	template <typename KeyType, typename FilterType>
	const KeyType* PickWeighted(const TMap<KeyType, float>& Weights, FRandomStream& Stream, FilterType Filter)
	{
		float TotalWeight = 0.0f;
		for (const TPair<KeyType, float>& Pair : Weights)
		{
			if (Filter(Pair.Key))
			{
				TotalWeight += FMath::Max(Pair.Value, 0.0f);
			}
		}
		if (TotalWeight <= 0.0f)
		{
			return nullptr;
		}

		float Roll = Stream.FRandRange(0.0f, TotalWeight);
		const KeyType* Picked = nullptr;
		for (const TPair<KeyType, float>& Pair : Weights)
		{
			if (!Filter(Pair.Key) || Pair.Value <= 0.0f)
			{
				continue;
			}
			Picked = &Pair.Key;
			Roll -= Pair.Value;
			if (Roll <= 0.0f)
			{
				break;
			}
		}
		return Picked;
	}
}

UMineralCatalog::UMineralCatalog()
{
	// Same values the tiles used before the catalog existed.
	auto AddSize = [this](EMineralAsteroidSize Size, float Scale, float Health, int32 Minerals)
	{
		FMineralAsteroidSizeStats& Stats = Sizes.Add(Size);
		Stats.Scale = Scale;
		Stats.Health = Health;
		Stats.Minerals = Minerals;
	};
	AddSize(EMineralAsteroidSize::Small, 2.0f, 50.0f, 2);
	AddSize(EMineralAsteroidSize::Medium, 4.0f, 100.0f, 3);
	AddSize(EMineralAsteroidSize::Large, 6.0f, 150.0f, 4);
	AddSize(EMineralAsteroidSize::Huge, 8.0f, 200.0f, 5);
}

const UMineralCatalog* UMineralCatalog::Get()
{
	const UMineralCatalog* Catalog = GetDefault<UGDTSurvivorSettings>()->MineralCatalog.LoadSynchronous();
	if (!Catalog)
	{
		UE_LOG(LogMineralCatalog, Error, TEXT("No mineral catalog set in Project Settings > Game > GDTSurvivor."));
	}
	return Catalog;
}

const FMineralZone* UMineralCatalog::FindZone(const FIntPoint& TileCoord) const
{
	// Straight line from the base tile center to this tile's center.
	const float Distance = FVector2D(TileCoord).Size();

	const FMineralZone* Found = nullptr;
	for (const FMineralZone& Zone : Zones)
	{
		if (Zone.MinDistanceInTiles <= Distance && (!Found || Zone.MinDistanceInTiles >= Found->MinDistanceInTiles))
		{
			Found = &Zone;
		}
	}
	return Found;
}

const UMineralDefinition* UMineralCatalog::PickMineral(const FMineralZone& Zone, FRandomStream& Stream) const
{
	const TObjectPtr<UMineralDefinition>* Picked = PickWeighted(Zone.MineralWeights, Stream,
		[](const TObjectPtr<UMineralDefinition>& Mineral) { return Mineral && Mineral->AsteroidClass; });
	return Picked ? Picked->Get() : nullptr;
}

bool UMineralCatalog::PickSize(const FMineralZone& Zone, EMineralAsteroidSize MinSize, EMineralAsteroidSize MaxSize,
	FRandomStream& Stream, EMineralAsteroidSize& OutSize) const
{
	const EMineralAsteroidSize* Picked = PickWeighted(Zone.SizeWeights, Stream, [&](EMineralAsteroidSize Size)
	{
		return Size >= MinSize && Size <= MaxSize && Sizes.Contains(Size);
	});
	if (!Picked)
	{
		return false;
	}
	OutSize = *Picked;
	return true;
}

bool UMineralCatalog::PickSizeInRange(EMineralAsteroidSize MinSize, EMineralAsteroidSize MaxSize,
	FRandomStream& Stream, EMineralAsteroidSize& OutSize) const
{
	TArray<EMineralAsteroidSize, TInlineAllocator<4>> Candidates;
	for (const TPair<EMineralAsteroidSize, FMineralAsteroidSizeStats>& Pair : Sizes)
	{
		if (Pair.Key >= MinSize && Pair.Key <= MaxSize)
		{
			Candidates.Add(Pair.Key);
		}
	}
	if (Candidates.IsEmpty())
	{
		return false;
	}
	// Map order is not guaranteed; sort so the same seed always gives the same size.
	Candidates.Sort();
	OutSize = Candidates[Stream.RandRange(0, Candidates.Num() - 1)];
	return true;
}

const UMineralDefinition* UMineralCatalog::FindById(FName MineralId) const
{
	for (const UMineralDefinition* Mineral : Minerals)
	{
		if (Mineral && Mineral->MineralId == MineralId)
		{
			return Mineral;
		}
	}
	return nullptr;
}

const UMineralDefinition* UMineralCatalog::FindByMineralClass(const UClass* MineralClass) const
{
	for (const UMineralDefinition* Mineral : Minerals)
	{
		if (Mineral && MineralClass && Mineral->GetMineralClass() == MineralClass)
		{
			return Mineral;
		}
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UMineralCatalog::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Error = [&](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	TSet<FName> Ids;
	TSet<const UClass*> MineralClasses;
	for (const UMineralDefinition* Mineral : Minerals)
	{
		if (!Mineral)
		{
			Error(TEXT("Empty entry in Minerals."));
			continue;
		}
		if (Mineral->MineralId.IsNone() || Ids.Contains(Mineral->MineralId))
		{
			Error(FString::Printf(TEXT("%s needs a unique MineralId."), *Mineral->GetName()));
		}
		Ids.Add(Mineral->MineralId);

		// Collected pieces are told apart by their class.
		const UClass* MineralClass = Mineral->GetMineralClass();
		if (!MineralClass)
		{
			Error(FString::Printf(TEXT("%s needs an AsteroidClass with a MineralClass."), *Mineral->GetName()));
		}
		else if (MineralClasses.Contains(MineralClass))
		{
			Error(FString::Printf(TEXT("%s breaks into the same mineral class as another mineral."), *Mineral->GetName()));
		}
		MineralClasses.Add(MineralClass);
	}

	if (Zones.IsEmpty() || Zones[0].MinDistanceInTiles > 0.0f)
	{
		Error(TEXT("The first zone has to start at MinDistanceInTiles = 0."));
	}
	for (int32 Index = 0; Index < Zones.Num(); ++Index)
	{
		if (Index > 0 && Zones[Index].MinDistanceInTiles <= Zones[Index - 1].MinDistanceInTiles)
		{
			Error(TEXT("Zones have to be sorted by MinDistanceInTiles, without duplicates."));
		}
		for (const TPair<TObjectPtr<UMineralDefinition>, float>& Pair : Zones[Index].MineralWeights)
		{
			if (Pair.Key && !Minerals.Contains(Pair.Key))
			{
				Error(FString::Printf(TEXT("Zone %d uses %s, which is not in Minerals."), Index, *Pair.Key->GetName()));
			}
		}
	}
	return Result;
}
#endif
