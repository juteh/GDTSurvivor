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
	
	float thrusterFXStrength = this->Force.Length() / this->ThrustSpeed * boosterScaleFactor * DeltaTime;
	thrusterFXStrength = FMath::Clamp(thrusterFXStrength, 0.f, 1.f);
	
	thrusterFXNiagaraComponent->SetFloatParameter(FName("Emissive_Boost"), thrusterFXStrength);
	thrusterFXNiagaraComponent->SetFloatParameter(FName("Smoke_Size"), thrusterFXStrength);
	thrusterFXNiagaraComponent->SetFloatParameter(FName("HeatHaze_Size"), thrusterFXStrength * heatHazeScaleFactor);
}

void APlayerSpaceShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	tickThrusterFX(DeltaTime);
	
}

void APlayerSpaceShipPawn::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &APlayerSpaceShipPawn::MovePlayer);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APlayerSpaceShipPawn::RotatePlayer);
	// IE_Pressed -> how the fire-button is used e.g. pressed or released
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APlayerSpaceShipPawn::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &APlayerSpaceShipPawn::StopFire);
}

void APlayerSpaceShipPawn::MovePlayer(float Value)
{
	this->Force = GetActorForwardVector() * Value * ThrustSpeed;
	// Name_None -> we don't use skeletal mesh with bones. Use force on the whole component
	// true -> accumulate force every new call of AddForce
	BoxComponent->AddForce(this->Force, NAME_None, true);
}

void APlayerSpaceShipPawn::RotatePlayer(float Value)
{
	FVector Torque = FVector(0, 0, Value * RotationSpeed);
	BoxComponent->AddTorqueInDegrees(Torque, NAME_None, true);
}

void APlayerSpaceShipPawn::StartFire()
{
	float FireRate = CurrentWeapon == 1 ? FireRateHomingMissile : FireRateStandardProjectile;
	GetWorld()->GetTimerManager().SetTimer(FireRateTimerHandle, this, &APlayerSpaceShipPawn::FireProjectile, FireRate, true, 0.0f);
}

void APlayerSpaceShipPawn::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
}

void APlayerSpaceShipPawn::FireProjectile()
{
	// true if ProjectileActorClass is set in BP_PlayerSpaceShipPawn
	if (ProjectileActorClass && HomingMissleActorClass)
	{
		const FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
		const FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;	
		// can also be zero but useful to define an originator e.g. for events to see who fired the projectile
		SpawnParameters.Instigator = GetInstigator();

		// create BP_Projectile
		AActor* SpawnedProjectile;
		if (CurrentWeapon == 0)
		{
			SpawnedProjectile = GetWorld()->SpawnActor<AActor>(ProjectileActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
		} else
		{
			SpawnedProjectile = GetWorld()->SpawnActor<AActor>(HomingMissleActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
			FindClosestActor();
			if (ClosestActor && SpawnedProjectile)
			{
				SetClosestActorForHomingMissile(SpawnedProjectile);
			}
		}
		
		if (SpawnedProjectile)
		{
			if (LaserShotSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LaserShotSound, GetActorLocation(), 0.3f);
			}
		}
	}
}


void APlayerSpaceShipPawn::HandleProjectileHit(AActor* HitActor, AActor* ProjectileActor)
{
	if (HitActor && ProjectileActor)
	{
		// find blueprints of asteroids by name
		FString ClassName = HitActor->GetClass()->GetName();
		if (ClassName.StartsWith(TEXT("BP_AsteroidsActor")))
		{
			FName EventName = FName("DestroyAsteroid");
			if (HitActor->FindFunction(EventName))
			{
				// trigger external event "DestroyAsteroid" of BP_AsteroidsActor
				// GLog -> standard logger for debugging by errors while event is triggered
				// bForceCallWithNonExec = true -> trigger the event regardless of whether it is private
				// function only works if event don't need parameters! Alternative use function ProcessEvent
				HitActor->CallFunctionByNameWithArguments(*EventName.ToString(), *GLog, nullptr, true);
			}
		}

		if (HitActor->ActorHasTag("level") || HitActor->ActorHasTag("enemy") || HitActor->ActorHasTag("asteroid"))
		{
			ProjectileActor->Destroy();
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing HitActor or ProjectileActor"));
	}
}

void APlayerSpaceShipPawn::FindClosestActor()
{
	TArray<AActor*> FoundActors;
	ClosestActor = nullptr;
	
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "enemy", FoundActors);

	for (AActor* CurrentActor: FoundActors)
	{
		if (!ClosestActor)
		{
			const float DistanceClosestActor = (this->GetActorLocation() - CurrentActor->GetActorLocation()).Length();
			if (DistanceClosestActor < MaxDistanceForSearchingActors)
			{
				ClosestActor = CurrentActor;
			}
		} else
		{
			const float DistanceClosestActor = (this->GetActorLocation() - ClosestActor->GetActorLocation()).Length();
			const float DistanceCurrentActor = (this->GetActorLocation() - CurrentActor->GetActorLocation()).Length();
			if (DistanceCurrentActor < DistanceClosestActor)
			{
				ClosestActor = CurrentActor;
			}
		}
	}
	FoundActors.Empty();
}
