#include "MetaProgressionSubsystem.h"

#include "MetaProgressionSaveGame.h"
#include "Engine/GameInstance.h"
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

void UMetaProgressionSubsystem::Save()
{
	UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
}

void UMetaProgressionSubsystem::HandleChanged()
{
	Save();
	OnMetaProgressionChanged.Broadcast();
}
