#include "Game/ShipGameState.h"

#include "Net/UnrealNetwork.h"

void AShipGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShipGameState, ActiveObjective);
}

void AShipGameState::SetActiveObjective(AObjective* Objective)
{
	ActiveObjective = Objective;
	OnActiveObjectiveChanged.Broadcast(ActiveObjective);
}

void AShipGameState::OnRep_ActiveObjective()
{
	OnActiveObjectiveChanged.Broadcast(ActiveObjective);
}
