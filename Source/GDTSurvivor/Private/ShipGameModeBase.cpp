#include "ShipGameModeBase.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ShipGameState.h"
#include "ShipStatsComponent.h"

void AShipGameModeBase::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	if (UShipStatsComponent* Stats = PlayerPawn ? PlayerPawn->FindComponentByClass<UShipStatsComponent>() : nullptr)
	{
		Stats->OnDeath.AddUniqueDynamic(this, &AShipGameModeBase::HandleShipDeath);
	}
}

void AShipGameModeBase::HandleShipDeath(UShipStatsComponent* Stats)
{
	// The controller is captured now, while the ship still exists. The decision runs one frame later,
	// so the ship's own death effects (sound, explosion) play before a defeat screen pauses the game.
	const APawn* Ship = Stats ? Cast<APawn>(Stats->GetOwner()) : nullptr;
	TWeakObjectPtr<APlayerController> PlayerController = Ship ? Ship->GetController<APlayerController>() : nullptr;

	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, PlayerController]()
	{
		OnPlayerShipDestroyed(PlayerController.Get());
	}));
}

void AShipGameModeBase::ReportObjectiveEvent(EObjectiveEvent Event)
{
	const AShipGameState* ShipGameState = GetGameState<AShipGameState>();
	if (AObjective* Objective = ShipGameState ? ShipGameState->GetActiveObjective() : nullptr)
	{
		Objective->ReportEvent(Event);
	}
}
