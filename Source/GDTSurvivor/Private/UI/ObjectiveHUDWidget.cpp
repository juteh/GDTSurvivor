#include "UI/ObjectiveHUDWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Objectives/Objective.h"
#include "Game/ShipGameState.h"

void UObjectiveHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AShipGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AShipGameState>() : nullptr;
	if (GameState)
	{
		BoundGameState = GameState;
		GameState->OnActiveObjectiveChanged.AddUniqueDynamic(this, &UObjectiveHUDWidget::HandleActiveObjectiveChanged);
	}
	HandleActiveObjectiveChanged(GameState ? GameState->GetActiveObjective() : nullptr);
}

void UObjectiveHUDWidget::NativeDestruct()
{
	UnbindFromObjective();
	if (AShipGameState* GameState = BoundGameState.Get())
	{
		GameState->OnActiveObjectiveChanged.RemoveDynamic(this, &UObjectiveHUDWidget::HandleActiveObjectiveChanged);
	}

	Super::NativeDestruct();
}

void UObjectiveHUDWidget::HandleActiveObjectiveChanged(AObjective* Objective)
{
	BindToObjective(Objective);
	HandleObjectiveChanged(Objective);
}

void UObjectiveHUDWidget::BindToObjective(AObjective* Objective)
{
	UnbindFromObjective();
	if (!Objective)
	{
		return;
	}

	BoundObjective = Objective;
	Objective->OnProgressChanged.AddUniqueDynamic(this, &UObjectiveHUDWidget::HandleObjectiveChanged);
	Objective->OnCompleted.AddUniqueDynamic(this, &UObjectiveHUDWidget::HandleObjectiveChanged);
}

void UObjectiveHUDWidget::UnbindFromObjective()
{
	if (AObjective* Objective = BoundObjective.Get())
	{
		Objective->OnProgressChanged.RemoveDynamic(this, &UObjectiveHUDWidget::HandleObjectiveChanged);
		Objective->OnCompleted.RemoveDynamic(this, &UObjectiveHUDWidget::HandleObjectiveChanged);
	}
	BoundObjective.Reset();
}

void UObjectiveHUDWidget::HandleObjectiveChanged(AObjective* Objective)
{
	const bool bComplete = Objective && Objective->IsComplete();

	TextBlock_Objective->SetText(Objective ? Objective->GetProgressText() : FText::GetEmpty());
	TextBlock_Objective->SetVisibility(Objective && !bComplete ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

	TextBlock_ObjectiveComplete->SetText(Objective ? Objective->GetCompletedText() : FText::GetEmpty());
	TextBlock_ObjectiveComplete->SetVisibility(bComplete ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}
