#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipStatsComponent.generated.h"

// Values of the player ship that upgrades (and later buffs) can change.
UENUM(BlueprintType)
enum class EShipStat : uint8
{
	MaxHealth,
	MaxShield,
	// Seconds after the last hit before the shield starts to recharge.
	ShieldRegenDelay,
	// Seconds per recharged shield point.
	ShieldRegenInterval,
	// Multiplier on every weapon's fire interval (lower = faster).
	FireIntervalMultiplier,
	// Flat damage added to every player projectile.
	DamageBonus,
	// Multiplier on the pickup radius of XP shards and minerals.
	PickupRangeMultiplier,

	Count UMETA(Hidden)
};

// One change to a stat. Final value = (base + sum of Additive) * product of Multiplier.
USTRUCT(BlueprintType)
struct GDTSURVIVOR_API FShipStatModifier
{
	GENERATED_BODY()

	// Where the modifier comes from (e.g. "LevelUp", "Permanent"), so it can be removed by source.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FName Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	EShipStat Stat = EShipStat::MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Additive = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Multiplier = 1.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShipResourceChanged, float, Current, float, Max);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShipStatChanged, EShipStat, Stat, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShipDeath);

/**
 * Single owner of the player ship's combat values: health, shield (incl. recharge), fire rate,
 * damage bonus and pickup range. Every stat is a base value plus modifiers from different
 * sources (level-ups, permanent upgrades, later buffs). Listeners (HUD, pawn) react to events;
 * nobody else stores a copy of these values. See Docs/Architecture.md.
 *
 * Permanent upgrades are applied here on BeginPlay. Level-up modifiers are pushed by the owning
 * pawn whenever the player state's chosen upgrades change.
 */
UCLASS(ClassGroup = (GDTSurvivor), meta = (BlueprintSpawnableComponent))
class GDTSURVIVOR_API UShipStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipStatsComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static const FName LevelUpSource;
	static const FName PermanentSource;

	// Stats

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetStat(EShipStat Stat) const;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddModifier(const FShipStatModifier& Modifier);

	// Returns the number of removed modifiers.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	int32 RemoveModifiersFromSource(FName Source);

	// Replaces all modifiers of Source with NewModifiers. Every affected stat is recalculated once,
	// so a raised maximum only fills health/shield by the net difference.
	void SetModifiersForSource(FName Source, const TArray<FShipStatModifier>& NewModifiers);

	// Health and shield

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetShield() const { return Shield; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetMaxShield() const { return MaxShield; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsDead() const { return Health <= 0.f; }

	// The shield absorbs damage first, the rest goes to health. Restarts the shield recharge.
	// Returns true if this hit killed the ship.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	bool ApplyIncomingDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void Heal(float Amount);

	// Restores health and shield carried over from the previous campaign level ("state" save).
	// Uses the saved fill ratio, so upgrades that raised the maximum since then are kept.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RestoreFromRunState(float SavedHealth, float SavedMaxHealth, float SavedShield, float SavedMaxShield);

	// Events

	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnShipResourceChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnShipResourceChanged OnShieldChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnShipStatChanged OnStatChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnShipDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	// Base values, before any modifier.

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Base", meta = (ClampMin = "1"))
	float BaseMaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Base", meta = (ClampMin = "0"))
	float BaseMaxShield = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Base", meta = (ClampMin = "0"))
	float BaseShieldRegenDelay = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats|Base", meta = (ClampMin = "0.01"))
	float BaseShieldRegenInterval = 0.1f;

private:
	float GetBaseValue(EShipStat Stat) const;

	// Lower bound of a stat, so stacked multipliers can't reach zero.
	static float GetMinValue(EShipStat Stat);

	void RecalculateStat(EShipStat Stat);

	void RecalculateStats(const TSet<EShipStat>& Stats);

	void ApplyPermanentUpgrades();

	void SetHealth(float NewHealth);
	void SetShield(float NewShield);

	void StartShieldRecharge();
	void RechargeShieldTick();

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Shield();

	TArray<FShipStatModifier> Modifiers;

	// Current value per EShipStat, recalculated whenever a modifier changes.
	TArray<float> StatValues;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float MaxHealth = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield)
	float Shield = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield)
	float MaxShield = 0.f;

	FTimerHandle ShieldRechargeTimer;
};
