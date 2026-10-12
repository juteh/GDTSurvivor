#include "Game/ShipPlayerState.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Minerals/MineralCatalog.h"
#include "Minerals/MineralDefinition.h"
#include "Progression/RunStateSubsystem.h"
#include "Upgrades/UpgradeCatalog.h"
#include "Upgrades/UpgradeDefinition.h"

void AShipPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShipPlayerState, Experience);
	DOREPLIFETIME(AShipPlayerState, Level);
	DOREPLIFETIME(AShipPlayerState, UpgradeStacks);
	DOREPLIFETIME(AShipPlayerState, CarriedMinerals);
	DOREPLIFETIME(AShipPlayerState, DeliveredMinerals);
}

AShipPlayerState* AShipPlayerState::GetShipPlayerState(const AActor* PlayerActor)
{
	if (const APawn* Pawn = Cast<APawn>(PlayerActor))
	{
		return Pawn->GetPlayerState<AShipPlayerState>();
	}
	if (const AController* Controller = Cast<AController>(PlayerActor))
	{
		return Controller->GetPlayerState<AShipPlayerState>();
	}
	return nullptr;
}

void AShipPlayerState::AddScore(int32 Amount)
{
	SetScore(GetScore() + Amount);
	OnScoreChanged.Broadcast(GetScoreAsInt());
}

void AShipPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Same rule as the ship's health: the run state belongs to the local campaign only.
	if (!HasAuthority() || GetNetMode() != NM_Standalone)
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const URunStateSubsystem* RunState = GameInstance ? GameInstance->GetSubsystem<URunStateSubsystem>() : nullptr;
	if (RunState && RunState->HasRun())
	{
		SetScore(RunState->GetScore());
		OnScoreChanged.Broadcast(GetScoreAsInt());
	}
}

void AShipPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	OnScoreChanged.Broadcast(GetScoreAsInt());
}

bool AShipPlayerState::AddExperience(int32 Amount)
{
	Experience += FMath::Max(0, Amount);

	bool bLeveledUp = false;
	while (Experience >= ExperiencePerLevel)
	{
		Experience -= ExperiencePerLevel;
		++Level;
		bLeveledUp = true;
		OnLevelUp.Broadcast(Level);
	}

	BroadcastExperience();
	return bLeveledUp;
}

void AShipPlayerState::AddUpgradeStack(const UUpgradeDefinition* Upgrade)
{
	if (!Upgrade)
	{
		return;
	}

	FUpgradeStack* Entry = UpgradeStacks.FindByPredicate([Upgrade](const FUpgradeStack& Stack) { return Stack.Upgrade == Upgrade; });
	if (!Entry)
	{
		Entry = &UpgradeStacks.AddDefaulted_GetRef();
		// Definitions are read-only assets; the stack only references them.
		Entry->Upgrade = const_cast<UUpgradeDefinition*>(Upgrade);
	}
	++Entry->Stacks;
	OnUpgradesChanged.Broadcast();
}

int32 AShipPlayerState::GetUpgradeStackCount(const UUpgradeDefinition* Upgrade) const
{
	const FUpgradeStack* Entry = UpgradeStacks.FindByPredicate([Upgrade](const FUpgradeStack& Stack) { return Stack.Upgrade == Upgrade; });
	return Entry ? Entry->Stacks : 0;
}

TArray<UUpgradeDefinition*> AShipPlayerState::GetLevelUpOptions(int32 Count) const
{
	TArray<UUpgradeDefinition*> Options;
	const UUpgradeCatalog* Catalog = UUpgradeCatalog::Get();
	if (!Catalog)
	{
		return Options;
	}

	for (UUpgradeDefinition* Upgrade : Catalog->Upgrades)
	{
		if (Upgrade && Upgrade->bOfferAsLevelUp && GetUpgradeStackCount(Upgrade) < Upgrade->MaxLevelUpStacks)
		{
			Options.Add(Upgrade);
		}
	}

	// Fisher-Yates shuffle, then keep the first Count.
	for (int32 Index = Options.Num() - 1; Index > 0; --Index)
	{
		Options.Swap(Index, FMath::RandRange(0, Index));
	}
	Options.SetNum(FMath::Min(Count, Options.Num()));
	return Options;
}

void AShipPlayerState::BroadcastExperience()
{
	OnExperienceChanged.Broadcast(Experience, ExperiencePerLevel, Level);
}

void AShipPlayerState::OnRep_Experience()
{
	BroadcastExperience();
}

void AShipPlayerState::OnRep_UpgradeStacks()
{
	OnUpgradesChanged.Broadcast();
}

namespace
{
	void AddMineralCount(TArray<FMineralCount>& Counts, UMineralDefinition* Mineral, int32 Amount)
	{
		if (FMineralCount* Existing = Counts.FindByPredicate([Mineral](const FMineralCount& Entry) { return Entry.Mineral == Mineral; }))
		{
			Existing->Count += Amount;
			return;
		}
		FMineralCount& Added = Counts.AddDefaulted_GetRef();
		Added.Mineral = Mineral;
		Added.Count = Amount;
	}
}

bool AShipPlayerState::CollectMineral(const AActor* MineralActor)
{
	if (!HasAuthority() || !MineralActor)
	{
		return false;
	}

	const UMineralCatalog* Catalog = UMineralCatalog::Get();
	const UMineralDefinition* Mineral = Catalog ? Catalog->FindByMineralClass(MineralActor->GetClass()) : nullptr;
	if (!Mineral)
	{
		UE_LOG(LogTemp, Warning, TEXT("CollectMineral: %s is not in the mineral catalog."), *MineralActor->GetClass()->GetName());
		return false;
	}

	// Definitions are read-only assets; the count only references them.
	AddMineralCount(CarriedMinerals, const_cast<UMineralDefinition*>(Mineral), 1);
	OnMineralsChanged.Broadcast();
	return true;
}

int32 AShipPlayerState::DeliverMinerals()
{
	if (!HasAuthority() || CarriedMinerals.IsEmpty())
	{
		return 0;
	}

	int32 Delivered = 0;
	for (const FMineralCount& Carried : CarriedMinerals)
	{
		AddMineralCount(DeliveredMinerals, Carried.Mineral, Carried.Count);
		Delivered += Carried.Count;
	}
	CarriedMinerals.Reset();
	OnMineralsChanged.Broadcast();
	return Delivered;
}

int32 AShipPlayerState::GetDeliveredMineralValue() const
{
	int32 Value = 0;
	for (const FMineralCount& Delivered : DeliveredMinerals)
	{
		if (Delivered.Mineral)
		{
			Value += Delivered.Count * Delivered.Mineral->Value;
		}
	}
	return Value;
}

void AShipPlayerState::OnRep_Minerals()
{
	OnMineralsChanged.Broadcast();
}
