#include "MetaProgressionSubsystem.h"

#include "MetaProgressionSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

const FString UMetaProgressionSubsystem::SaveSlotName = TEXT("metaprogression");

void UMetaProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveGame = Cast<UMetaProgressionSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
	if (!SaveGame)
	{
		SaveGame = Cast<UMetaProgressionSaveGame>(UGameplayStatics::CreateSaveGameObject(UMetaProgressionSaveGame::StaticClass()));
		SaveGame->Money = StartingMoney;
		Save();
	}
}

int32 UMetaProgressionSubsystem::GetMoney() const
{
	return SaveGame->Money;
}

int32 UMetaProgressionSubsystem::GetUpgradeLevel(EUpgradeType Type) const
{
	return SaveGame->UpgradeLevels.FindRef(Type);
}

int32 UMetaProgressionSubsystem::GetMaxUpgradeLevel(EUpgradeType Type) const
{
	return MaxUpgradeLevel;
}

int32 UMetaProgressionSubsystem::GetUpgradeCost(EUpgradeType Type) const
{
	return UpgradeCost;
}

bool UMetaProgressionSubsystem::CanBuyUpgrade(EUpgradeType Type) const
{
	return GetUpgradeLevel(Type) < GetMaxUpgradeLevel(Type) && GetMoney() >= GetUpgradeCost(Type);
}

bool UMetaProgressionSubsystem::BuyUpgrade(EUpgradeType Type)
{
	if (!CanBuyUpgrade(Type))
	{
		return false;
	}

	const int32 Cost = GetUpgradeCost(Type);
	SaveGame->Money -= Cost;
	SaveGame->SpentMoney += Cost;
	SaveGame->UpgradeLevels.FindOrAdd(Type)++;
	HandleChanged();
	return true;
}

void UMetaProgressionSubsystem::ResetUpgrades()
{
	SaveGame->Money += SaveGame->SpentMoney;
	SaveGame->SpentMoney = 0;
	SaveGame->UpgradeLevels.Reset();
	HandleChanged();
}

void UMetaProgressionSubsystem::AddMoney(int32 Amount)
{
	SaveGame->Money = FMath::Max(0, SaveGame->Money + Amount);
	HandleChanged();
}

TArray<uint8> UMetaProgressionSubsystem::GetPermanentUpgradeStacks(const UObject* WorldContextObject)
{
	TArray<uint8> Stacks;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMetaProgressionSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UMetaProgressionSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return Stacks;
	}

	for (const TPair<EUpgradeType, int32>& Entry : Subsystem->SaveGame->UpgradeLevels)
	{
		for (int32 Level = 0; Level < Entry.Value; ++Level)
		{
			Stacks.Add(static_cast<uint8>(Entry.Key));
		}
	}
	return Stacks;
}

void UMetaProgressionSubsystem::Save()
{
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
}

void UMetaProgressionSubsystem::HandleChanged()
{
	Save();
	OnMetaProgressionChanged.Broadcast();
}
