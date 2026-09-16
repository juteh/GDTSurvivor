// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerSpaceShipPawn.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UDamageType;
class UPrimitiveComponent;
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
	
protected:
	virtual void BeginPlay() override;

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

	// LevelUp upgrades

	// Reduces FireRateStandardProjectile/FireRateHomingMissile by PercentIncrease/100, clamped so fire rate can't reach zero or below.
	UFUNCTION(BlueprintCallable, Category = "LevelUp")
	void IncreaseFireRate(float PercentIncrease);

	UFUNCTION(BlueprintCallable, Category = "LevelUp")
	void IncreaseDamageBonus(float FlatAmount);

	// Flat damage added to every projectile spawned by FireProjectile(), on top of the projectile class's own default damage.
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	float DamageBonus = 0.f;

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
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRateStandardProjectile = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRateHomingMissile = 0.7f;

	// utilities

	UPROPERTY(EditAnywhere, Category = "Utilities", meta = (AllowPrivateAccess = "true"))
	float MaxDistanceForSearchingActors = 1500;

	UPROPERTY(EditAnywhere, Category = "Utilities", meta = (AllowPrivateAccess = "true"))
	float MaxDistanceForSearchingActorsForRadar = 3000;
};