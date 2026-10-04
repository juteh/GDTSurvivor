#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MineralAsteroid.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraSystem;
class URotatingMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Big mineral rock. Takes damage (ApplyDamage from projectiles) and breaks apart into
 * MineralCount small minerals of its type when its health runs out.
 * One Blueprint child per mineral type sets the mesh and MineralClass.
 * Size = actor scale (mesh and collision are authored for scale 1); tiles pick the size.
 */
UCLASS()
class GDTSURVIVOR_API AMineralAsteroid : public AActor
{
	GENERATED_BODY()

public:
	AMineralAsteroid();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// Radius in the XY plane (incl. actor scale), used by the tiles to find a free spot.
	float GetFootprintRadius() const;

	// Overrides health and mineral count for the size picked by the tile.
	// Call between SpawnActorDeferred and FinishSpawning; the size itself is the actor scale.
	void SetSizeStats(float InMaxHealth, int32 InMineralCount);

	UFUNCTION(BlueprintPure, Category = "Asteroid")
	float GetHealth() const { return Health; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asteroid")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asteroid")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asteroid")
	TObjectPtr<URotatingMovementComponent> Rotation;

	// Defaults for an asteroid placed by hand (scale 8). Tiles override both per size,
	// see FMineralAsteroidSpawnSettings::Sizes on the EndlessTileManager.
	UPROPERTY(EditAnywhere, Category = "Asteroid", meta = (ClampMin = "1"))
	float MaxHealth = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Asteroid|Minerals", meta = (ClampMin = "0"))
	int32 MineralCount = 5;

	// Small mineral spawned when the asteroid breaks (BP_Mineral_Iconos, ...).
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Minerals")
	TSubclassOf<AActor> MineralClass;

	// Minerals appear within FootprintRadius * this factor around the center ...
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Minerals", meta = (ClampMin = "0"))
	float ScatterRadiusFactor = 0.5f;

	// ... and are pushed outwards with this speed (randomized +-25%), fading out
	// via DriftDamping on their MineralMagnetComponent.
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Minerals", meta = (ClampMin = "0"))
	float MineralLaunchSpeed = 900.0f;

	// Max random spin in degrees per second (looks like the small minerals, just slower).
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid")
	float MaxSpinSpeed = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects")
	TObjectPtr<UNiagaraSystem> BreakEffect;

	// The explosion effect ignores its spawn scale, so big asteroids get several explosions
	// instead: count = round(actor scale * ExplosionsPerScale), at least 1 (scale 8 -> 4).
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects", meta = (ClampMin = "0"))
	float ExplosionsPerScale = 0.5f;

	// Extra explosions are spread within FootprintRadius * this factor.
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects", meta = (ClampMin = "0"))
	float ExplosionSpread = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects")
	TObjectPtr<USoundBase> BreakSound;

	// Additive overlay with "Damage_Intensity" (M_DamageFlashOverlay), same red as the enemies.
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects")
	TObjectPtr<UMaterialInterface> DamageFlashMaterial;

	// Damage_Intensity right after a hit; fades linearly to 0.
	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects", meta = (ClampMin = "0"))
	float DamageFlashIntensity = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Asteroid|Effects", meta = (ClampMin = "0.01"))
	float DamageFlashDuration = 0.3f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void Break();

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
