// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpaceShipPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

// Sets default values
APlayerSpaceShipPawn::APlayerSpaceShipPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));

	RootComponent = CapsuleComponent;

	SpaceshipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipMesh"));
	SpaceshipMesh->SetupAttachment(RootComponent);
	SpaceshipMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SpaceshipMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CapsuleComponent);
	CameraBoom->TargetArmLength = 500.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = true;
}

void APlayerSpaceShipPawn::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerSpaceShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if no keys are pressed, reduce speed over time
	if (!IsMovingVertical && !IsMovingHorizontal)
	{
		FVector DecelerationVector = -CurrentVelocity.GetSafeNormal() * Deceleration * DeltaTime;
		
		// stop movement
		if (DecelerationVector.Size() > CurrentVelocity.Size())
		{
			CurrentVelocity = FVector::ZeroVector;
		}
		// reduce speed
		else
		{
			CurrentVelocity += DecelerationVector;
		}
	}

	CurrentVelocity = FMath::Clamp(CurrentVelocity.Size(), 0.0f, MaxSpeed) * CurrentVelocity.GetSafeNormal();

	// CurrentVelocity set new location and CurrentVelocity changed while usin inputs in MoveVertical() and MoveHorizontal()
	FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
	// true -> use collision
	SetActorLocation(NewLocation, true);

	// Rotation of SpaceShip 
	if (!CurrentVelocity.IsNearlyZero()) {
		FRotator NewRotation = CurrentVelocity.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, RotationSpeed));
	}

}

void APlayerSpaceShipPawn::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &APlayerSpaceShipPawn::MoveVertical);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APlayerSpaceShipPawn::MoveHorizontal);
}

void APlayerSpaceShipPawn::MoveVertical(float Value)
{
	if (Value != 0.0f)
	{
		FVector Direction = FVector(1, 0, 0);
		CurrentVelocity += Direction * Value * Acceleration * GetWorld()->GetDeltaSeconds();
		IsMovingVertical = true;
	}
	else
	{
		IsMovingVertical = false;
	}
}

void APlayerSpaceShipPawn::MoveHorizontal(float Value)
{
	if (Value != 0.0f)
	{
		FVector Direction = FVector(0, 1, 0);
		CurrentVelocity += Direction * Value * Acceleration * GetWorld()->GetDeltaSeconds();
		IsMovingHorizontal = true;
	}
	else
	{
		IsMovingHorizontal = false;
	}
}
