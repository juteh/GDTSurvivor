// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerSpaceShipPawn.generated.h"

UCLASS()
class TOONTANKS_API APlayerSpaceShipPawn : public APawn
{
	GENERATED_BODY()

public:
	APlayerSpaceShipPawn();
	void BeginThrusterFX();

	// Functions in EventGraph
	
	UFUNCTION(BlueprintCallable, Server, reliable, Category = "Combat")
	void HandleProjectileHit(AActor* HitActor, AActor* ProjectileActor);

	UFUNCTION(BlueprintCallable, Category = "Utilities")
	void FindClosestActor(float searchDistance);

    UFUNCTION(BlueprintCallable, Category = "Utilities")
	float GetRadarRotationAngle();

	UFUNCTION(BlueprintImplementableEvent , Category = "Combat")
	void SetClosestActorForHomingMissile(AActor* HomingMissileActor);

	UPROPERTY(BlueprintReadWrite, Category="Combat")
	int CurrentWeapon = 0;

	UPROPERTY(BlueprintReadWrite, Category="Combat")
	AActor* ClosestActor;

protected:
	virtual void BeginPlay() override;
	
	void tickThrusterFX(float DeltaTime);

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, reliable)
	void MovePlayer(float Value);

	UFUNCTION(Server, reliable)
	void RotatePlayer(float Value);

	UFUNCTION(Server, reliable)
	void StartFire();
	UFUNCTION(Server, reliable)
	void StopFire();

	UFUNCTION(Server, reliable)
	void FireProjectile();

	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* RootSceneComponent;


	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UBoxComponent* BoxComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* SpaceshipMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FollowCamera;

	// Moving-Parameters


	// only rotation speed of the ship. Has no effect on movementspeed
	UPROPERTY(EditAnywhere, Category = "Movement")
	float RotationSpeed = 700.0f;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float ThrustSpeed = 1700.0f;

	float RotateClockwise = false;
	
	FVector Force;
 
	// Sound-Parameters

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UAudioComponent* AudioComponent;

	// to use this we have to set in BP_PlayerSpaceShipPawn under ClassDefaults of ProjectileActorClass the Blueprint BP_Projectile
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AActor> ProjectileActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AActor> HomingMissleActorClass;
	
	// combat
	
	// spawn point for projectile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* ProjectileSpawnPoint;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* LaserShotSound;

	FTimerHandle FireRateTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRateStandardProjectile = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireRateHomingMissile = 0.7f;

	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* thrusterFXNiagaraSystem;

	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* thrusterFXNiagaraComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* thrusterFXNiagaraComponentLeft;

	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* thrusterFXNiagaraComponentRight;

	// utilities

	UPROPERTY(EditAnywhere, Category = "Utilities", meta = (AllowPrivateAccess = "true"))
	float MaxDistanceForSearchingActors = 1500;

	UPROPERTY(EditAnywhere, Category = "Utilities", meta = (AllowPrivateAccess = "true"))
	float MaxDistanceForSearchingActorsForRadar = 3000;
	
};