// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpaceShipPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

// Sets default values
APlayerSpaceShipPawn::APlayerSpaceShipPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	//RootComponent = RootSceneComponent;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollisionComponent"));
	RootComponent = CapsuleComponent;
	//CapsuleComponent->SetupAttachment(RootComponent);
	//CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//CapsuleComponent->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	//CapsuleComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

	SpaceshipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipMesh"));
	SpaceshipMesh->SetupAttachment(CapsuleComponent);

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



	// Rotation of SpaceShip 
	if (!CurrentVelocity.IsNearlyZero()) {
		FRotator NewRotation = CurrentVelocity.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, RotationSpeed));
	}
	
	// CurrentVelocity set new location and CurrentVelocity changed while using inputs in MoveVertical() and MoveHorizontal()
	FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
	// true -> use collision
	SetActorLocation(NewLocation, true);
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
		//float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);
		//FVector DeltaLocation = FVector::ZeroVector;
		//DeltaLocation.X = Value * DeltaTime * Acceleration;
		// true -> use collision
		//AddActorLocalOffset(DeltaLocation, true);

		
		//FRotator DeltaRotation = FRotator::ZeroRotator;
		//DeltaRotation.Yaw = Value * RotationSpeed * DeltaTime;
		//AddActorLocalRotation(DeltaRotation, true);
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
		//float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);
		//FVector DeltaLocation = FVector::ZeroVector;
		//DeltaLocation.Y = Value * DeltaTime * Acceleration;
		// true -> use collision
		//AddActorLocalOffset(DeltaLocation, true);

		//FRotator DeltaRotation = FRotator::ZeroRotator;
		//DeltaRotation.Yaw = Value * RotationSpeed * DeltaTime;
		//AddActorLocalRotation(DeltaRotation, true);
	}
	else
	{
		IsMovingHorizontal = false;
	}
}
