// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerSpaceShipPawn.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class TOONTANKS_API APlayerSpaceShipPawn : public APawn
{
	GENERATED_BODY()

public:
	APlayerSpaceShipPawn();

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

	void BeginThrusterFX();
	
	void TickThrusterFX(float DeltaTime);

	UNiagaraSystem* ThrusterFXNiagaraSystem;

	UNiagaraComponent* CreateThrusterFX(const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	
	UNiagaraComponent* ThrusterFXNiagaraComponent;
	UNiagaraComponent* ThrusterFXNiagaraComponentLeft;
	UNiagaraComponent* ThrusterFXNiagaraComponentRight;
	UNiagaraComponent* ThrusterFXNiagaraComponentLeftFront;
	UNiagaraComponent* ThrusterFXNiagaraComponentRightFront;

	//UFUNCTION(Server, reliable)
	void UpdateThrusterParameters(UNiagaraComponent* Component, float Strength);


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

	//UFUNCTION(Server, reliable)
	//void UpdateThrusterSettings(float pThrusterFXStrengthCentral, float pThrusterFXStrengthLeft, float pThrusterFXStrengthRight, float pThrusterFXStrengthLeftFront, float pThrusterFXStrengthRightFront);

	
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

 
	// Sound-Parameters

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UAudioComponent* AudioComponent;

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

	UAudioComponent* ThrusterAudioComponent;

	bool ThrusterSoundPlaying = false;

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