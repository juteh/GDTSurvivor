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

	// set CaspuleCollide for RootComponent of blueprint
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollisionComponent"));
	RootComponent = CapsuleComponent;

	// Add and attach Mesh of SpaceShip to CapsuleCollision 
	SpaceshipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipMesh"));
	SpaceshipMesh->SetupAttachment(CapsuleComponent);

	// create camera attached to SpringArm
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CapsuleComponent);
	CameraBoom->TargetArmLength = 1500.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);

	// create background-music and activate
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = true;

	// create spawnpoint for projectile
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(SpaceshipMesh);
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
		// ensures that the vector is only normalised if its length is greater than 0. If the vector has a length of 0, a zero vector is simply returned.
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

	// CurrentVelocity set new location and CurrentVelocity changed while using inputs in MoveVertical() and MoveHorizontal()
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
	// IE_Pressed -> how the fire-button is used e.g pressed or released
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APlayerSpaceShipPawn::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &APlayerSpaceShipPawn::StopFire);
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

void APlayerSpaceShipPawn::StartFire()
{
	GetWorld()->GetTimerManager().SetTimer(FireRateTimerHandle, this, &APlayerSpaceShipPawn::FireProjectile, FireRate, true, 0.0f);
}

void APlayerSpaceShipPawn::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
}

void APlayerSpaceShipPawn::FireProjectile()
{
	// true if ProjectileActorClass is set in BP_PlayerSpaceShipPawn
	if (ProjectileActorClass) {
		const FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
		const FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		// can also be zero but useful to define an originator e.g. for events to see who fired the projectile
		SpawnParameters.Instigator = GetInstigator();

		// create BP_Projectile
		AActor* SpawnedProjectile = GetWorld()->SpawnActor<AActor>(ProjectileActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
		
		if (SpawnedProjectile)
		{
			OnProjectileCreate(SpawnedProjectile);
			if (LaserShotSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LaserShotSound, GetActorLocation(), 0.3f);
			}
		}
	}
}

void APlayerSpaceShipPawn::HandleProjectileHit(AActor* ProjectileActor, AActor* HitActor)
{
	if (HitActor)
	{
		// TODO: Mit Tags arbeiten
		// HitActor->ActorHasTag()
		UE_LOG(LogTemp, Warning, TEXT("Projectile hit Actor %s"), *HitActor->GetActorLabel());
		if (ProjectileActor)
		{
			if (HitActor->ActorHasTag("level"))
			{
				OnProjectileDestroy(ProjectileActor, HitActor);
			}
		}
	}
}
