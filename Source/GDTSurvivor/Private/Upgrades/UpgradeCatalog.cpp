#include "Upgrades/UpgradeCatalog.h"

#include "Game/GDTSurvivorSettings.h"
#include "Upgrades/UpgradeDefinition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogUpgradeCatalog, Log, All);

const UUpgradeCatalog* UUpgradeCatalog::Get()
{
	const UUpgradeCatalog* Catalog = GetDefault<UGDTSurvivorSettings>()->UpgradeCatalog.LoadSynchronous();
	if (!Catalog)
	{
		UE_LOG(LogUpgradeCatalog, Error, TEXT("No upgrade catalog set in Project Settings > Game > GDTSurvivor."));
	}
	return Catalog;
}

const UUpgradeDefinition* UUpgradeCatalog::FindById(FName UpgradeId) const
{
	for (const UUpgradeDefinition* Upgrade : Upgrades)
	{
		if (Upgrade && Upgrade->UpgradeId == UpgradeId)
		{
			return Upgrade;
		}
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UUpgradeCatalog::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	TSet<FName> Ids;
	for (const UUpgradeDefinition* Upgrade : Upgrades)
	{
		if (!Upgrade)
		{
			Context.AddError(FText::FromString(TEXT("Empty entry in Upgrades.")));
			Result = EDataValidationResult::Invalid;
		}
		else if (Upgrade->UpgradeId.IsNone() || Ids.Contains(Upgrade->UpgradeId))
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("%s needs a unique UpgradeId."), *Upgrade->GetName())));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			Ids.Add(Upgrade->UpgradeId);
		}
	}
	return Result;
}
#endif
