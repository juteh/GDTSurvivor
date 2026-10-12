#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WreckContainer.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;
class UStaticMeshComponent;

/** One kind of pickup a container can drop. */
USTRUCT(BlueprintType)
struct FWreckLootEntry
{
	GENERATED_BODY()

	// Pickup to spawn (BP_XPPickup, BP_HealthPack, BP_Mineral_Iconos, ...).
	UPROPERTY(EditAnywhere, Category = "Loot")
	TSubclassOf<AActor> ActorClass;

	// Chance that this entry drops at all (1 = always).
	UPROPERTY(EditAnywhere, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.0f;

	// If it drops: a random count between Min and Max.
	UPROPERTY(EditAnywhere, Category = "Loot", meta = (ClampMin = "0"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, Category = "Loot", meta = (ClampMin = "0"))
	int32 MaxCount = 1;
};

/**
 * Cargo container in a wreck. Takes damage (ApplyDamage from projectiles) and bursts open
 * when its health runs out, scattering the pickups rolled from Loot.
 * Placed in tile maps like any other Blueprint actor; the tile spawns it on the server.
 * Blueprint children (BP_WreckContainer_*) only set mesh, size and loot.
 */
UCLASS()
class GDTSURVIVOR_API AWreckContainer : public AActor
{
	GENERATED_BODY()

public:
	AWreckContainer();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Container")
	float GetHealth() const { return Health; }

	// Root and collision. Authored for actor scale 1 (100uu cube); size it with the actor scale.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Container", meta = (ClampMin = "1"))
	float MaxHealth = 60.0f;

	// Every entry is rolled on its own when the container breaks.
	UPROPERTY(EditAnywhere, Category = "Container|Loot")
	TArray<FWreckLootEntry> Loot;

	// Pickups appear within this distance around the center ...
	UPROPERTY(EditDefaultsOnly, Category = "Container|Loot", meta = (ClampMin = "0"))
	float ScatterRadius = 80.0f;

	// ... and pickups with a MineralMagnetComponent are pushed outwards with this speed (+-25%).
	UPROPERTY(EditDefaultsOnly, Category = "Container|Loot", meta = (ClampMin = "0"))
	float LootLaunchSpeed = 700.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Container|Effects")
	TObjectPtr<UNiagaraSystem> BreakEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Container|Effects")
	TObjectPtr<USoundBase> BreakSound;

	// Additive overlay with "Damage_Intensity" (M_DamageFlashOverlay), same as the mineral asteroids.
	UPROPERTY(EditDefaultsOnly, Category = "Container|Effects")
	TObjectPtr<UMaterialInterface> DamageFlashMaterial;

	// Damage_Intensity right after a hit; fades linearly to 0.
	UPROPERTY(EditDefaultsOnly, Category = "Container|Effects", meta = (ClampMin = "0"))
	float DamageFlashIntensity = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Container|Effects", meta = (ClampMin = "0.01"))
	float DamageFlashDuration = 0.3f;

protected:
	virtual void BeginPlay() override;

private:
	void Break();

	void SpawnLoot();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayBreakEffects();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayDamageFlash();

	void SetFlashIntensity(float Intensity);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DamageFlashInstance;

	float FlashTimeRemaining = 0.0f;

	float Health = 0.0f;

	bool bBroken = false;
};
