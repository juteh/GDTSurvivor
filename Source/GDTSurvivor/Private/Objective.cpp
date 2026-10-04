#include "Objective.h"

#include "Net/UnrealNetwork.h"

AObjective::AObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AObjective, Progress);
	DOREPLIFETIME(AObjective, Required);
	DOREPLIFETIME(AObjective, StoredProgress);
	DOREPLIFETIME(AObjective, bIsComplete);
}

void AObjective::StartObjective(int32 InRequired)
{
	Required = InRequired;
	Progress = 0;
	bIsComplete = false;
	OnProgressChanged.Broadcast(this);
}

void AObjective::ReportEvent(EObjectiveEvent Event)
{
	if (Event != EObjectiveEvent::None && Event == CountedEvent)
	{
		AddProgress(1);
	}
}

void AObjective::AddProgress(int32 Amount)
{
	if (bIsComplete)
	{
		return;
	}

	Progress += Amount;
	OnProgressChanged.Broadcast(this);

	// Same rule as the former Blueprint objectives: complete once the progress exceeds Required.
	if (bCanComplete && Progress > Required)
	{
		bIsComplete = true;
		OnCompleted.Broadcast(this);
	}
}

void AObjective::StoreProgress()
{
	StoredProgress += Progress;
	Progress = 0;
	OnProgressChanged.Broadcast(this);
}

FText AObjective::GetProgressText() const
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Current"), Progress);
	Arguments.Add(TEXT("Required"), Required);
	Arguments.Add(TEXT("Stored"), StoredProgress);
	return FText::Format(ProgressFormat, Arguments);
}

void AObjective::OnRep_Progress()
{
	OnProgressChanged.Broadcast(this);
}

void AObjective::OnRep_IsComplete()
{
	if (bIsComplete)
	{
		OnCompleted.Broadcast(this);
	}
}
