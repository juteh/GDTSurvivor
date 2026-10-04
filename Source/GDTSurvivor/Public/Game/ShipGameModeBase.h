#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Objectives/Objective.h"
#include "ShipGameModeBase.generated.h"

/**
 * Parent class of BP_GameMode_Base. Listens to every player ship's UShipStatsComponent::OnDeath
 * (the ship only reports its death, the GameMode decides what it means) and forwards objective
 * events to the active objective.
 */
UCLASS()
class GDTSURVIVOR_API AShipGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

	// Counts the event on the active objective (if that objective tracks it).
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void ReportObjectiveEvent(EObjectiveEvent Event);

protected:
	// A player's ship was destroyed. Implemented in BP_GameMode_Base (lose / win checks).
	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void OnPlayerShipDestroyed(APlayerController* PlayerController);

private:
	// Runs inside the killing hit, before the ship destroys itself, so its controller is still known.
	// Calls OnPlayerShipDestroyed on the next frame.
	UFUNCTION()
	void HandleShipDeath(class UShipStatsComponent* Stats);
};
