#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ObjectiveHUDWidget.generated.h"

class AObjective;
class AShipGameState;
class UTextBlock;

/**
 * Parent class of WBP_HUD_Objective. Shows the active objective's progress text and, once it is
 * complete, its completed text. Follows AShipGameState::OnActiveObjectiveChanged and the objective's
 * own events, so the GameMode never touches this widget.
 */
UCLASS(Abstract)
class GDTSURVIVOR_API UObjectiveHUDWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_Objective;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TextBlock_ObjectiveComplete;

private:
	UFUNCTION()
	void HandleActiveObjectiveChanged(AObjective* Objective);

	UFUNCTION()
	void HandleObjectiveChanged(AObjective* Objective);

	void BindToObjective(AObjective* Objective);
	void UnbindFromObjective();

	TWeakObjectPtr<AShipGameState> BoundGameState;
	TWeakObjectPtr<AObjective> BoundObjective;
};
