#include "MetaProgressionSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "MetaProgressionSaveGame.h"
#include "UpgradeDefinition.h"

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
	else if (SaveGame->SpentMoney > 0 && SaveGame->UpgradeLevelsById.IsEmpty())
	{
		// Saves from before upgrade ids stored their levels per enum value, which no longer exists.
		// Refund what was spent instead of losing it.
		ResetUpgrades();
	}
}

int32 UMetaProgressionSubsystem::GetMoney() const
{
	return SaveGame->Money;
}

int32 UMetaProgressionSubsystem::GetUpgradeLevel(const UUpgradeDefinition* Upgrade) const
{
	return Upgrade ? SaveGame->UpgradeLevelsById.FindRef(Upgrade->UpgradeId) : 0;
}

bool UMetaProgressionSubsystem::CanBuyUpgrade(const UUpgradeDefinition* Upgrade) const
{
	return Upgrade
		&& Upgrade->bSellAsPermanent
		&& GetUpgradeLevel(Upgrade) < Upgrade->MaxPermanentLevel
		&& GetMoney() >= Upgrade->PermanentCost;
}

bool UMetaProgressionSubsystem::BuyUpgrade(const UUpgradeDefinition* Upgrade)
{
	if (!CanBuyUpgrade(Upgrade))
	{
		return false;
	}

	SaveGame->Money -= Upgrade->PermanentCost;
	SaveGame->SpentMoney += Upgrade->PermanentCost;
	SaveGame->UpgradeLevelsById.FindOrAdd(Upgrade->UpgradeId)++;
	HandleChanged();
	return true;
}

void UMetaProgressionSubsystem::ResetUpgrades()
{
	SaveGame->Money += SaveGame->SpentMoney;
	SaveGame->SpentMoney = 0;
	SaveGame->UpgradeLevelsById.Reset();
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
