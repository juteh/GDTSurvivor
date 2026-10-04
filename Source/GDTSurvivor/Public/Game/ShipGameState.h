#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Objectives/Objective.h"
#include "ShipGameState.generated.h"

/**
 * Parent class of BP_GameState_Base. Holds the level's active objective so every client (HUD,
 * result board) can read it and listen to it. Timer and intensity still live in the Blueprint.
 */
UCLASS()
class GDTSURVIVOR_API AShipGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called by the GameMode after spawning the objective.
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void SetActiveObjective(AObjective* Objective);

	UFUNCTION(BlueprintPure, Category = "Objective")
	AObjective* GetActiveObjective() const { return ActiveObjective; }

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnObjectiveChanged OnActiveObjectiveChanged;

private:
	UFUNCTION()
	void OnRep_ActiveObjective();

	UPROPERTY(ReplicatedUsing = OnRep_ActiveObjective)
	TObjectPtr<AObjective> ActiveObjective;
};
