// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpaceShipPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Niagara/Public/NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values
APlayerSpaceShipPawn::APlayerSpaceShipPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// set BoxCollider for RootComponent of blueprint
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollisionComponent"));
	RootComponent = BoxComponent;

	// Add and attach Mesh of SpaceShip to CapsuleCollision 
	SpaceshipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpaceshipMesh"));
	SpaceshipMesh->SetupAttachment(BoxComponent);
	
	// create camera attached to SpringArm
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(BoxComponent);
	CameraBoom->TargetArmLength = 1500.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);

	// create thruster fx
	NiagaraSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("ThrusterFXPoint"));
	NiagaraSceneComp->SetupAttachment(SpaceshipMesh);
	
	// create background-music and activate
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = true;

	// create spawn point for projectile
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(SpaceshipMesh);
}

void APlayerSpaceShipPawn::BeginThrusterFX()
{
	FString NiagaraPath = "/Game/GDTSurvivor/Effects/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Blue.NS_RocketExhaust_Blue";
	thrusterFXNiagaraSystem = Cast<UNiagaraSystem>(StaticLoadObject(UNiagaraSystem::StaticClass(), nullptr, *NiagaraPath));
	thrusterFXNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(thrusterFXNiagaraSystem,  this->RootComponent, NAME_None, FVector(-50.f,00.f,0.f), FRotator(0,180,00.f), FVector(0.5, 0.5, 0.5), EAttachLocation::Type::KeepRelativeOffset, true, ENCPoolMethod::None);

	thrusterFXNiagaraComponent->InitializeSystem();
	thrusterFXNiagaraComponent->Activate(true);
}

void APlayerSpaceShipPawn::BeginPlay()
{
	Super::BeginPlay();
	
	BeginThrusterFX();
}

void APlayerSpaceShipPawn::tickThrusterFX(float DeltaTime)
{
	const float boosterScaleFactor = 100.f;
	const float heatHazeScaleFactor = 10.f;
	
	float thrusterFXStrength = (this->CurrentVelocity * DeltaTime).Length() / this->MaxSpeed * boosterScaleFactor;
	thrusterFXStrength = FMath::Clamp(thrusterFXStrength, 0.f, 1.f);
	
	thrusterFXNiagaraComponent->SetFloatParameter(FName("Emissive_Boost"), thrusterFXStrength);
	thrusterFXNiagaraComponent->SetFloatParameter(FName("Smoke_Size"), thrusterFXStrength);
	thrusterFXNiagaraComponent->SetFloatParameter(FName("HeatHaze_Size"), thrusterFXStrength * heatHazeScaleFactor);
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

	tickThrusterFX(DeltaTime);
	
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
	// IE_Pressed -> how the fire-button is used e.g. pressed or released
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
	if (ProjectileActorClass)
	{
		const FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
		const FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		// can also be zero but useful to define an originator e.g. for events to see who fired the projectile
		SpawnParameters.Instigator = GetInstigator();

		// create BP_Projectile
		const AActor* SpawnedProjectile = GetWorld()->SpawnActor<AActor>(ProjectileActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
		
		if (SpawnedProjectile)
		{
			if (LaserShotSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LaserShotSound, GetActorLocation(), 0.3f);
			}
		}
	}
}

void APlayerSpaceShipPawn::HandleProjectileHit(AActor* ProjectileActor, AActor* HitActor)
{
	if (HitActor && ProjectileActor && (HitActor->ActorHasTag("level") || HitActor->ActorHasTag("enemy")))
	{
		OnProjectileDestroy(ProjectileActor, HitActor);
	}
}
