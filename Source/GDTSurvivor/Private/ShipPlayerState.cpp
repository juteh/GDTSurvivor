#include "ShipPlayerState.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

AShipPlayerState::AShipPlayerState()
{
	// NumEnums() includes the generated _MAX entry.
	UpgradeStacks.Init(0, StaticEnum<EUpgradeType>()->NumEnums() - 1);
}

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

void AShipPlayerState::AddUpgradeStack(uint8 Type)
{
	if (UpgradeStacks.IsValidIndex(Type))
	{
		++UpgradeStacks[Type];
		OnUpgradesChanged.Broadcast();
	}
}

int32 AShipPlayerState::GetUpgradeStackCount(uint8 Type) const
{
	return UpgradeStacks.IsValidIndex(Type) ? UpgradeStacks[Type] : 0;
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
