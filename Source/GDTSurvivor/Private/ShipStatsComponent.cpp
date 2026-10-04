#include "ShipStatsComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MetaProgressionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "RunStateSubsystem.h"
#include "TimerManager.h"
#include "UpgradeCatalog.h"
#include "UpgradeDefinition.h"

const FName UShipStatsComponent::LevelUpSource = TEXT("LevelUp");
const FName UShipStatsComponent::PermanentSource = TEXT("Permanent");

UShipStatsComponent::UShipStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	StatValues.Init(0.f, static_cast<int32>(EShipStat::Count));
}

void UShipStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShipStatsComponent, Health);
	DOREPLIFETIME(UShipStatsComponent, MaxHealth);
	DOREPLIFETIME(UShipStatsComponent, Shield);
	DOREPLIFETIME(UShipStatsComponent, MaxShield);
}

void UShipStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Raising the maximum from 0 also fills health and shield (see RecalculateStat).
	for (int32 Index = 0; Index < static_cast<int32>(EShipStat::Count); ++Index)
	{
		RecalculateStat(static_cast<EShipStat>(Index));
	}

	if (GetOwner()->HasAuthority())
	{
		ApplyPermanentUpgrades();
		ApplyRunState();
	}
}

float UShipStatsComponent::GetStat(EShipStat Stat) const
{
	return StatValues[static_cast<int32>(Stat)];
}

void UShipStatsComponent::AddModifier(const FShipStatModifier& Modifier)
{
	Modifiers.Add(Modifier);
	RecalculateStat(Modifier.Stat);
}

int32 UShipStatsComponent::RemoveModifiersFromSource(FName Source)
{
	TSet<EShipStat> ChangedStats;
	const int32 Removed = Modifiers.RemoveAll([&](const FShipStatModifier& Modifier)
	{
		if (Modifier.Source != Source)
		{
			return false;
		}
		ChangedStats.Add(Modifier.Stat);
		return true;
	});

	RecalculateStats(ChangedStats);
	return Removed;
}

void UShipStatsComponent::SetModifiersForSource(FName Source, const TArray<FShipStatModifier>& NewModifiers)
{
	TSet<EShipStat> ChangedStats;
	Modifiers.RemoveAll([&](const FShipStatModifier& Modifier)
	{
		if (Modifier.Source != Source)
		{
			return false;
		}
		ChangedStats.Add(Modifier.Stat);
		return true;
	});

	for (FShipStatModifier Modifier : NewModifiers)
	{
		Modifier.Source = Source;
		ChangedStats.Add(Modifier.Stat);
		Modifiers.Add(Modifier);
	}

	RecalculateStats(ChangedStats);
}

bool UShipStatsComponent::ApplyIncomingDamage(float Damage)
{
	if (Damage <= 0.f || IsDead())
	{
		return false;
	}

	const float Absorbed = FMath::Min(Shield, Damage);
	SetShield(Shield - Absorbed);

	const float Remaining = Damage - Absorbed;
	if (Remaining > 0.f)
	{
		SetHealth(Health - Remaining);
	}

	if (IsDead())
	{
		GetWorld()->GetTimerManager().ClearTimer(ShieldRechargeTimer);
		OnDeath.Broadcast();
		return true;
	}

	StartShieldRecharge();
	return false;
}

void UShipStatsComponent::Heal(float Amount)
{
	if (Amount > 0.f && !IsDead())
	{
		SetHealth(Health + Amount);
	}
}

void UShipStatsComponent::ApplyRunState()
{
	// The run state belongs to the local campaign; multiplayer matches always start fresh.
	if (GetNetMode() != NM_Standalone)
	{
		return;
	}

	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	const URunStateSubsystem* RunState = GameInstance ? GameInstance->GetSubsystem<URunStateSubsystem>() : nullptr;
	if (!RunState || !RunState->HasRun())
	{
		return;
	}

	// At least one point, so a level can never start with a dead ship.
	SetHealth(FMath::Max(1.f, MaxHealth * RunState->GetHealthFraction()));
	SetShield(MaxShield * RunState->GetShieldFraction());
	StartShieldRecharge();
}

float UShipStatsComponent::GetBaseValue(EShipStat Stat) const
{
	switch (Stat)
	{
	case EShipStat::MaxHealth:              return BaseMaxHealth;
	case EShipStat::MaxShield:              return BaseMaxShield;
	case EShipStat::ShieldRegenDelay:       return BaseShieldRegenDelay;
	case EShipStat::ShieldRegenInterval:    return BaseShieldRegenInterval;
	case EShipStat::FireIntervalMultiplier: return 1.f;
	case EShipStat::DamageBonus:            return 0.f;
	case EShipStat::PickupRangeMultiplier:  return 1.f;
	default:                                return 0.f;
	}
}

float UShipStatsComponent::GetMinValue(EShipStat Stat)
{
	switch (Stat)
	{
	case EShipStat::MaxHealth:              return 1.f;
	case EShipStat::ShieldRegenInterval:    return 0.05f;
	case EShipStat::FireIntervalMultiplier: return 0.05f;
	default:                                return 0.f;
	}
}

void UShipStatsComponent::RecalculateStat(EShipStat Stat)
{
	float Additive = 0.f;
	float Multiplier = 1.f;
	for (const FShipStatModifier& Modifier : Modifiers)
	{
		if (Modifier.Stat == Stat)
		{
			Additive += Modifier.Additive;
			Multiplier *= Modifier.Multiplier;
		}
	}

	const float NewValue = FMath::Max(GetMinValue(Stat), (GetBaseValue(Stat) + Additive) * Multiplier);
	StatValues[static_cast<int32>(Stat)] = NewValue;

	// A higher maximum also fills the resource by the same amount (like picking up a bigger tank).
	if (Stat == EShipStat::MaxHealth)
	{
		const float Delta = NewValue - MaxHealth;
		MaxHealth = NewValue;
		Health = FMath::Clamp(Health + FMath::Max(0.f, Delta), 0.f, MaxHealth);
		OnHealthChanged.Broadcast(Health, MaxHealth);
	}
	else if (Stat == EShipStat::MaxShield)
	{
		const float Delta = NewValue - MaxShield;
		MaxShield = NewValue;
		Shield = FMath::Clamp(Shield + FMath::Max(0.f, Delta), 0.f, MaxShield);
		OnShieldChanged.Broadcast(Shield, MaxShield);
	}

	OnStatChanged.Broadcast(Stat, NewValue);
}

void UShipStatsComponent::RecalculateStats(const TSet<EShipStat>& Stats)
{
	for (const EShipStat Stat : Stats)
	{
		RecalculateStat(Stat);
	}
}

void UShipStatsComponent::ApplyPermanentUpgrades()
{
	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	const UMetaProgressionSubsystem* MetaProgression = GameInstance ? GameInstance->GetSubsystem<UMetaProgressionSubsystem>() : nullptr;
	const UUpgradeCatalog* Catalog = UUpgradeCatalog::Get();
	if (!MetaProgression || !Catalog)
	{
		return;
	}

	TArray<FShipStatModifier> PermanentModifiers;
	for (const UUpgradeDefinition* Upgrade : Catalog->Upgrades)
	{
		if (Upgrade)
		{
			Upgrade->AppendModifiers(MetaProgression->GetUpgradeLevel(Upgrade), PermanentSource, PermanentModifiers);
		}
	}
	SetModifiersForSource(PermanentSource, PermanentModifiers);
}

void UShipStatsComponent::SetHealth(float NewHealth)
{
	const float Clamped = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (Clamped != Health)
	{
		Health = Clamped;
		OnHealthChanged.Broadcast(Health, MaxHealth);
	}
}

void UShipStatsComponent::SetShield(float NewShield)
{
	const float Clamped = FMath::Clamp(NewShield, 0.f, MaxShield);
	if (Clamped != Shield)
	{
		Shield = Clamped;
		OnShieldChanged.Broadcast(Shield, MaxShield);
	}
}

void UShipStatsComponent::StartShieldRecharge()
{
	if (Shield >= MaxShield)
	{
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(ShieldRechargeTimer, this, &UShipStatsComponent::RechargeShieldTick,
		GetStat(EShipStat::ShieldRegenInterval), true, GetStat(EShipStat::ShieldRegenDelay));
}

void UShipStatsComponent::RechargeShieldTick()
{
	if (IsDead() || Shield >= MaxShield)
	{
		GetWorld()->GetTimerManager().ClearTimer(ShieldRechargeTimer);
		return;
	}
	SetShield(Shield + 1.f);
}

void UShipStatsComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UShipStatsComponent::OnRep_Shield()
{
	OnShieldChanged.Broadcast(Shield, MaxShield);
}
