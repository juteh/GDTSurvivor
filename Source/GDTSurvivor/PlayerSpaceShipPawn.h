// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerSpaceShipPawn.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UDamageType;
class UPrimitiveComponent;
class UWidget;
struct FHitResult;

UCLASS()
class GDTSURVIVOR_API APlayerSpaceShipPawn : public APawn
{
	GENERATED_BODY()

public:
	APlayerSpaceShipPawn();

	// Functions in EventGraph
	
	UFUNCTION(BlueprintCallable, Server, reliable, Category = "Combat")
	void HandleProjectileHit(AActor* HitActor, AActor* ProjectileActor, UActorComponent* HitComponent);

	// Bound to BoxComponent's OnComponentHit in the constructor. Deals ram damage to both
	// ships when the player physically collides (blocks) with an actor tagged "enemy".
	UFUNCTION()
	void OnShipCollision(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// Damage applied to both ships on a ram collision.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float RamDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<UDamageType> RamDamageTypeClass;

	// Minimum time between ram-damage applications against the same actor, so sustained
	// contact (sliding along a hull) doesn't reapply damage every physics tick.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float RamDamageCooldown = 0.5f;

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	AActor* FindClosestActor(float MaxDistanceForSearching, FName Tag="enemy");

    UFUNCTION(BlueprintCallable, Category = "Utilities")
	float GetRadarRotationAngle(FName Tag="mineral");

	// Rotates the HUD compass arrows towards the closest mineral asteroid and the dropzone.
	// An arrow without a target is hidden. Called every frame by WBP_HUD.
	UFUNCTION(BlueprintCallable, Category = "Utilities")
	void UpdateRadarArrows(UWidget* AsteroidArrow, UWidget* DropzoneArrow);

	// Actor tag of the mineral dropzone (MineralsDropzone Blueprint).
	UPROPERTY(EditDefaultsOnly, Category = "Utilities")
	FName DropzoneTag = TEXT("dropzone");

	// Direction the arrow texture points to when not rotated, in screen degrees
	// (0 = right, 90 = down, -90 = up). T_CompassArrow points up.
	UPROPERTY(EditDefaultsOnly, Category = "Utilities")
	float RadarArrowTextureDirection = -90.0f;

	// Radius (HUD units) of the compass ring the arrows move on, around their designer position.
	// T_CompassRing shown at 80x80 has its ring line at radius 36.
	UPROPERTY(EditDefaultsOnly, Category = "Utilities")
	float RadarRingRadius = 36.0f;

	// Tint of the compass arrows (applied if the arrow widget is an Image).
	UPROPERTY(EditDefaultsOnly, Category = "Utilities")
	FLinearColor AsteroidArrowColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, Category = "Utilities")
	FLinearColor DropzoneArrowColor = FLinearColor(0.1f, 0.4f, 1.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Utilities")
	AActor* FindClosestTarget(const FName Tag ="enemy");

	UFUNCTION(BlueprintImplementableEvent , Category = "Combat")
	void SetClosestActorForHomingMissile(AActor* HomingMissileActor);

	UPROPERTY(BlueprintReadWrite, Category="Combat")
	int CurrentWeapon = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ShootAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UStaticMeshComponent* SpaceshipMesh;

	// Health, shield, fire rate, damage bonus and pickup range incl. all upgrades.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UShipStatsComponent* ShipStats;
	
protected:
	virtual void BeginPlay() override;

	// Keeps ShipStats' level-up modifiers in sync with the upgrades chosen in the player state.
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;

	UFUNCTION()
	void HandleUpgradesChanged();

	void BeginThrusterFX();
	
	void TickThrusterFX(float DeltaTime);

	UNiagaraSystem* ThrusterFXNiagaraSystem;

	UNiagaraComponent* CreateThrusterFX(const FVector& Location, const FRotator& Rotation, const FVector& Scale) const;
	
	UNiagaraComponent* ThrusterFXNiagaraComponent;
	UNiagaraComponent* ThrusterFXNiagaraComponentLeft;
	UNiagaraComponent* ThrusterFXNiagaraComponentRight;
	UNiagaraComponent* ThrusterFXNiagaraComponentLeftFront;
	UNiagaraComponent* ThrusterFXNiagaraComponentRightFront;

	//UFUNCTION(Server, reliable)
	static void UpdateThrusterParameters(UNiagaraComponent* Component, float Strength);


	UPROPERTY( replicated )
	float CurrentThrusterVolume = 0.0f;
	UPROPERTY( replicated )
	float ThrusterFXStrengthCentral = 0.0f;
	UPROPERTY( replicated )
	float ThrusterFXStrengthLeft = 0.0f;
	UPROPERTY( replicated )
	float ThrusterFXStrengthRight = 0.0f;
	UPROPERTY( replicated )
	float ThrusterFXStrengthLeftFront = 0.0f;
	UPROPERTY( replicated )
	float ThrusterFXStrengthRightFront = 0.0f;
	UPROPERTY( replicated )
	FVector Force;
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentThrusterVolume() {}
	UFUNCTION()
	void OnRep_ThrusterFXStrengthCentral() {}
	UFUNCTION()
	void OnRep_ThrusterFXStrengthLeft() {}
	UFUNCTION()
	void OnRep_ThrusterFXStrengthRight() {}
	UFUNCTION()
	void OnRep_ThrusterFXStrengthLeftFront() {}
	UFUNCTION()
	void OnRep_ThrusterFXStrengthRightFront() {}
	UFUNCTION()
	void OnRep_Force() {}


	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPlaySound(UAudioComponent* Sound, bool play);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySound(UAudioComponent* Sound, bool play);
	void PlaySoundOnNetwork(UAudioComponent* Sound, bool play);

	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, reliable)
	void MovePlayer(float Value);

	UFUNCTION(Server, reliable)
	void MovePlayerEnhanced(const FInputActionValue& Value);

	UFUNCTION(Server, reliable)
	void RotatePlayer(float Value);

	UFUNCTION(Server, reliable)
	void StartFire();
	UFUNCTION(Server, reliable)
	void StopFire();

	UFUNCTION(NetMulticast, Reliable)
	void FireProjectileSound();
	
	UFUNCTION(Server, reliable)
	void FireProjectile();


private:
	// Timestamp (GetWorld()->GetTimeSeconds()) of the last ram-damage application per other actor, for RamDamageCooldown.
	TMap<AActor*, float> LastRamDamageTimeByActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* RootSceneComponent;


	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UBoxComponent* BoxComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FollowCamera;

	// Moving-Parameters


	// only rotation speed of the ship. Has no effect on movement speed
	UPROPERTY(EditAnywhere, Category = "Movement")
	float RotationSpeed = 700.0f;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float ThrustSpeed = 1700.0f;
	
	// Sound-Parameters
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UAudioComponent* ThrusterAudioComponent;
	bool ThrusterSoundPlaying = false;

	// to use this we have to set in BP_PlayerSpaceShipPawn under ClassDefaults of ProjectileActorClass the Blueprint BP_Projectile
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AActor> ProjectileActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AActor> HomingMissileActorClass;
	
	// combat
	
	// spawn point for projectile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* ProjectileSpawnPoint;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* LaserShotSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* ThrusterSound;

	FTimerHandle FireRateTimerHandle;
	
	// Base seconds between shots; multiplied by the FireIntervalMultiplier stat of ShipStats.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRateStandardProjectile = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRateHomingMissile = 0.7f;

	// utilities

	UPROPERTY(EditAnywhere, Category = "Utilities", meta = (AllowPrivateAccess = "true"))
	float MaxDistanceForSearchingActors = 1500;

	UPROPERTY(EditAnywhere, Category = "Utilities", meta = (AllowPrivateAccess = "true"))
	float MaxDistanceForSearchingActorsForRadar = 3000;

	// Arrow rotation (for SetRenderTransformAngle) from the ship to a world location (world space).
	float GetRadarAngleToLocation(const FVector& TargetLocation) const;

	// Direction from ship to target in screen degrees (0 = right, 90 = down), world-space based.
	float GetRadarWorldDirection(const FVector& TargetLocation) const;

	// Same, but measured on screen (ship and target projected); falls back to the world direction.
	float GetRadarScreenDirection(const FVector& TargetLocation) const;

	// Moves the arrow onto the compass ring towards ScreenDirection and points it outwards.
	void PlaceRadarArrow(UWidget* Arrow, float ScreenDirection) const;

	AActor* FindClosestMineralAsteroid() const;

	// The dropzone lives in the start tile and is destroyed while that tile is recycled,
	// so this may return null; then the compass points to the start tile center instead.
	AActor* FindDropzone();

	TWeakObjectPtr<AActor> CachedDropzone;

	// Avoids searching all actors every frame while no dropzone exists.
	double NextDropzoneSearchTime = 0.0;
};