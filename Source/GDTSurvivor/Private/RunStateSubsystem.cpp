#include "RunStateSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RunStateSaveGame.h"

const FString URunStateSubsystem::SaveSlotName = TEXT("runstate");

void URunStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveGame = Cast<URunStateSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
}

URunStateSubsystem* URunStateSubsystem::GetRunState(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<URunStateSubsystem>() : nullptr;
}

bool URunStateSubsystem::HasRun() const
{
	return SaveGame != nullptr;
}

void URunStateSubsystem::StartNewRun()
{
	SaveGame = Cast<URunStateSaveGame>(UGameplayStatics::CreateSaveGameObject(URunStateSaveGame::StaticClass()));
	Save();
}

void URunStateSubsystem::DeleteRun()
{
	SaveGame = nullptr;
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);
}

FString URunStateSubsystem::GetPlayerName() const
{
	return SaveGame ? SaveGame->PlayerName : FString();
}

void URunStateSubsystem::SetPlayerName(const FString& PlayerName)
{
	if (SaveGame)
	{
		SaveGame->PlayerName = PlayerName;
		Save();
	}
}

int32 URunStateSubsystem::GetCompletedLevels() const
{
	return SaveGame ? SaveGame->CompletedLevels : 0;
}

int32 URunStateSubsystem::GetScore() const
{
	return SaveGame ? SaveGame->Score : 0;
}

float URunStateSubsystem::GetHealthFraction() const
{
	return SaveGame ? SaveGame->HealthFraction : 1.f;
}

float URunStateSubsystem::GetShieldFraction() const
{
	return SaveGame ? SaveGame->ShieldFraction : 1.f;
}

void URunStateSubsystem::RecordLevelCompleted(int32 Score, float HealthFraction, float ShieldFraction)
{
	if (!SaveGame)
	{
		StartNewRun();
	}

	SaveGame->Score = Score;
	SaveGame->HealthFraction = FMath::Clamp(HealthFraction, 0.f, 1.f);
	SaveGame->ShieldFraction = FMath::Clamp(ShieldFraction, 0.f, 1.f);
	++SaveGame->CompletedLevels;
	Save();
}

void URunStateSubsystem::Save()
{
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
}
