#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Objective.generated.h"

// Things that happen in a match and may count towards an objective.
UENUM(BlueprintType)
enum class EObjectiveEvent : uint8
{
	None,
	MineralCollected,
	EnemyDestroyed
};

class AObjective;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveChanged, AObjective*, Objective);

/**
 * Parent class of BP_ObjectiveBase and its children (one per objective type). The Blueprint children
 * only set data: which event they count, whether they can be completed, and the texts.
 * The GameMode spawns the level's objective and reports events to it; the objective HUD listens to it.
 */
UCLASS(Abstract)
class GDTSURVIVOR_API AObjective : public AActor
{
	GENERATED_BODY()

public:
	AObjective();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void StartObjective(int32 InRequired);

	// Adds one progress step if this objective counts the event.
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void ReportEvent(EObjectiveEvent Event);

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void AddProgress(int32 Amount);

	// Endless mode: moves the current progress into the stored amount (minerals delivered at the dropzone).
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void StoreProgress();

	UFUNCTION(BlueprintPure, Category = "Objective")
	int32 GetProgress() const { return Progress; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	int32 GetRequired() const { return Required; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	int32 GetStoredProgress() const { return StoredProgress; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	bool IsComplete() const { return bIsComplete; }

	// ProgressFormat with {Current}, {Required} and {Stored} filled in.
	UFUNCTION(BlueprintPure, Category = "Objective")
	FText GetProgressText() const;

	UFUNCTION(BlueprintPure, Category = "Objective")
	const FText& GetCompletedText() const { return CompletedText; }

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnObjectiveChanged OnProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnObjectiveChanged OnCompleted;

protected:
	// Which event counts as one progress step.
	UPROPERTY(EditDefaultsOnly, Category = "Objective")
	EObjectiveEvent CountedEvent = EObjectiveEvent::None;

	// False for endless objectives that only collect.
	UPROPERTY(EditDefaultsOnly, Category = "Objective")
	bool bCanComplete = true;

	// Shown while the objective runs. Placeholders: {Current}, {Required}, {Stored}.
	UPROPERTY(EditDefaultsOnly, Category = "Objective", meta = (MultiLine = true))
	FText ProgressFormat;

	UPROPERTY(EditDefaultsOnly, Category = "Objective")
	FText CompletedText;

private:
	UFUNCTION()
	void OnRep_Progress();

	UFUNCTION()
	void OnRep_IsComplete();

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 Progress = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 Required = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Progress)
	int32 StoredProgress = 0;

	UPROPERTY(ReplicatedUsing = OnRep_IsComplete)
	bool bIsComplete = false;
};
