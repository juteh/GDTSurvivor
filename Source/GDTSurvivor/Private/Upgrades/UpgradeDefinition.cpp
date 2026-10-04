#include "Upgrades/UpgradeDefinition.h"

void UUpgradeDefinition::AppendModifiers(int32 Stacks, FName Source, TArray<FShipStatModifier>& OutModifiers) const
{
	for (int32 Stack = 0; Stack < Stacks; ++Stack)
	{
		for (const FUpgradeEffect& Effect : Effects)
		{
			FShipStatModifier& Modifier = OutModifiers.AddDefaulted_GetRef();
			Modifier.Source = Source;
			Modifier.Stat = Effect.Stat;
			Modifier.Additive = Effect.Additive;
			Modifier.Multiplier = Effect.Multiplier;
		}
	}
}
