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

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveVertical(float Value);
	void MoveHorizontal(float Value);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCapsuleComponent* CapsuleComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* SpaceshipMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FollowCamera;

	// Moving-Parameters

	bool IsMovingVertical = false;
	bool IsMovingHorizontal = false;

	// increase speed over time until maxspeed
	UPROPERTY(EditAnywhere, Category = "Movement")
	float Acceleration = 2000.0f;

	// reduce speed while no movement-keys are pressed until zero
	UPROPERTY(EditAnywhere, Category = "Movement")
	float Deceleration = 500.0f;

	// limit fpr acceleration
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxSpeed = 1000.0f;

	// only rotation speed of the ship. Has no effect on movementspeed
	UPROPERTY(EditAnywhere, Category = "Movement")
	float RotationSpeed = 5.0f;

	FVector CurrentVelocity;

	// Sound-Parameters

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UAudioComponent* AudioComponent;
};