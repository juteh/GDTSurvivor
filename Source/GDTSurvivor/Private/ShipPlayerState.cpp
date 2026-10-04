#include "ShipPlayerState.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "UpgradeCatalog.h"
#include "UpgradeDefinition.h"

void AShipPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShipPlayerState, Experience);
	DOREPLIFETIME(AShipPlayerState, Level);
	DOREPLIFETIME(AShipPlayerState, UpgradeStacks);
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

void AShipPlayerState::RestoreScoreFromRunState(int32 SavedScore)
{
	SetScore(SavedScore);
	OnScoreChanged.Broadcast(GetScoreAsInt());
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
