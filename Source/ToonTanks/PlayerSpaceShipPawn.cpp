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
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// Sets default values
APlayerSpaceShipPawn::APlayerSpaceShipPawn()
{
	this->bReplicates=true;
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
	
	// create background-music and activate
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = true;

	// create spawn point for projectile
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(SpaceshipMesh);

}

UNiagaraComponent* APlayerSpaceShipPawn::CreateThrusterFX(const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		ThrusterFXNiagaraSystem, 
		RootComponent, 
		NAME_None,
		Location, 
		Rotation,
		Scale, 
		EAttachLocation::Type::KeepRelativeOffset,
		true, 
		ENCPoolMethod::None
	);

	NiagaraComponent->InitializeSystem();
	NiagaraComponent->Activate(true);
	
	return NiagaraComponent;
}

void APlayerSpaceShipPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlayerSpaceShipPawn, ThrusterFXStrengthCentral);
	DOREPLIFETIME(APlayerSpaceShipPawn, ThrusterFXStrengthLeft);
	DOREPLIFETIME(APlayerSpaceShipPawn, ThrusterFXStrengthRight);
	DOREPLIFETIME(APlayerSpaceShipPawn, ThrusterFXStrengthLeftFront);
	DOREPLIFETIME(APlayerSpaceShipPawn, ThrusterFXStrengthRightFront);
	DOREPLIFETIME(APlayerSpaceShipPawn, Force);
}


void APlayerSpaceShipPawn::UpdateThrusterParameters(UNiagaraComponent* ThrusterComponent, float ThrusterStrength)
{
	constexpr float HeatHazeScaleFactor = 10.f;
    
	ThrusterComponent->SetFloatParameter(FName("Emissive_Boost"), ThrusterStrength);
	ThrusterComponent->SetFloatParameter(FName("Smoke_Size"), ThrusterStrength);
	ThrusterComponent->SetFloatParameter(FName("HeatHaze_Size"), ThrusterStrength * HeatHazeScaleFactor);
}

void APlayerSpaceShipPawn::BeginThrusterFX()
{
	const FString NiagaraPath = "/Game/GDTSurvivor/Effects/RocketThrusterExhaustFX/FX/NS_RocketExhaust_Blue.NS_RocketExhaust_Blue";
	ThrusterFXNiagaraSystem = Cast<UNiagaraSystem>(
		StaticLoadObject(UNiagaraSystem::StaticClass(), nullptr, *NiagaraPath)
	);
	
	ThrusterFXNiagaraComponent = CreateThrusterFX(FVector(-50,0,0),
		FRotator(0,180,0), FVector(0.5, 0.5, 0.5)
	);
	ThrusterFXNiagaraComponentLeft = CreateThrusterFX(FVector(-50,-40,0),
		FRotator(0,170,0),FVector(0.3, 0.3, 0.3)
	);
	ThrusterFXNiagaraComponentRight = CreateThrusterFX(FVector(-50,40,0),
		FRotator(0,190,0),FVector(0.3, 0.3, 0.3)
	);
	ThrusterFXNiagaraComponentLeftFront = CreateThrusterFX(FVector(0,-40,0),
		FRotator(0,-10,0),FVector(0.3, 0.3, 0.3)
	);
	ThrusterFXNiagaraComponentRightFront = CreateThrusterFX(FVector(0,40,0),
		FRotator(0,10,0),FVector(0.3, 0.3, 0.3)
	);
}

void APlayerSpaceShipPawn::BeginPlay()
{
	Super::BeginPlay();
	
	BeginThrusterFX();

	// Set up thruster-sound component, but do not play it yet
	if (ThrusterSound)
	{
		ThrusterAudioComponent = UGameplayStatics::SpawnSoundAttached(
			ThrusterSound,
			RootComponent,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			true,
			1.5
		);
	}
	
}

// Function to play sound
void APlayerSpaceShipPawn::PlaySoundOnNetwork(UAudioComponent* Sound, bool play)
{
	if (HasAuthority()) // Check if this is the server
	{
		// Call the multicast function to play the sound on all clients
		MulticastPlaySound(Sound, play);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("PlaySoundNetwork Authority"));;
		// If not the server, request the server to play the sound
		ServerPlaySound(Sound, play);
	}
}

// Server function to handle sound playback
void APlayerSpaceShipPawn::ServerPlaySound_Implementation(UAudioComponent* Sound, bool play)
{
	MulticastPlaySound(Sound, play);
}

bool APlayerSpaceShipPawn::ServerPlaySound_Validate(UAudioComponent* Sound, bool play)
{
	return true; // Add validation logic if needed
}

// Multicast function to play sound on all clients
void APlayerSpaceShipPawn::MulticastPlaySound_Implementation(UAudioComponent* Sound, bool play)
{
	if (Sound)
	{
		if (play)
			Sound->Play();
		else
			Sound->Stop();
	}
}

void APlayerSpaceShipPawn::TickThrusterFX(const float DeltaTime)
{
	// constexpr set value while compiling not to run time. Just for the efficiency
	constexpr float BoosterScaleFactor = 100.f;
	
	float ThrusterFXStrength = this->Force.Length() / this->ThrustSpeed * BoosterScaleFactor * DeltaTime;
	ThrusterFXStrength = FMath::Clamp(ThrusterFXStrength, 0.f, 1.f);

	float ThrusterFXRotationStrength = FMath::Abs(BoxComponent->GetPhysicsAngularVelocityInDegrees().Z)/ this->ThrustSpeed * BoosterScaleFactor * DeltaTime;
	ThrusterFXRotationStrength = FMath::Clamp(ThrusterFXRotationStrength, 0.f, 1.f);

	ThrusterFXStrengthCentral = 0.f;
	ThrusterFXStrengthLeft = 0.f;
	ThrusterFXStrengthRight = 0.f;
	ThrusterFXStrengthLeftFront = 0.f;
	ThrusterFXStrengthRightFront = 0.f;
	
	if(GetActorForwardVector().Dot(this->Force) > 0.f)		//We are moving forward
	{
		ThrusterFXStrengthCentral = ThrusterFXStrength;	
	} else {												//... and backward
		ThrusterFXStrengthLeftFront = ThrusterFXStrength;
		ThrusterFXStrengthRightFront = ThrusterFXStrength;
	}
	
	if(BoxComponent->GetPhysicsAngularVelocityInDegrees().Z>0.f)		//We are moving left
	{
		ThrusterFXStrengthLeft = ThrusterFXRotationStrength;
	} else {															//We are moving right
		ThrusterFXStrengthRight = ThrusterFXRotationStrength;
	}
	
	// play thruster sound
	bool shouldPlaySound = !FMath::IsNearlyZero(ThrusterFXStrengthCentral) ||
					  !FMath::IsNearlyZero(ThrusterFXStrengthLeft) ||
					  !FMath::IsNearlyZero(ThrusterFXStrengthRight) ||
					  !FMath::IsNearlyZero(ThrusterFXStrengthLeftFront) ||
					  !FMath::IsNearlyZero(ThrusterFXStrengthRightFront);
		
	CurrentThrusterVolume = ThrusterFXStrength;
	ThrusterAudioComponent->AdjustVolume(2,CurrentThrusterVolume,EAudioFaderCurve::Linear);
	
	UpdateThrusterParameters(ThrusterFXNiagaraComponent, ThrusterFXStrengthCentral);
	UpdateThrusterParameters(ThrusterFXNiagaraComponentLeft, ThrusterFXStrengthLeft);
	UpdateThrusterParameters(ThrusterFXNiagaraComponentRight, ThrusterFXStrengthRight);
	UpdateThrusterParameters(ThrusterFXNiagaraComponentLeftFront, ThrusterFXStrengthLeftFront);
	UpdateThrusterParameters(ThrusterFXNiagaraComponentRightFront, ThrusterFXStrengthRightFront);
}

void APlayerSpaceShipPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickThrusterFX(DeltaTime);
}

void APlayerSpaceShipPawn::SetupPlayerInputComponent(UInputComponent * PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	// deprecated binding
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &APlayerSpaceShipPawn::MovePlayer);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &APlayerSpaceShipPawn::RotatePlayer);
	// IE_Pressed -> how the fire-button is used e.g. pressed or released
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APlayerSpaceShipPawn::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &APlayerSpaceShipPawn::StopFire);

	if (EnhancedInputComponent && PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
		
		if (ShootAction)
		{
			EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &APlayerSpaceShipPawn::StartFire);
			EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &APlayerSpaceShipPawn::StopFire);
		}
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerSpaceShipPawn::MovePlayerEnhanced);
		}
	}
}

void APlayerSpaceShipPawn::MovePlayer_Implementation(float Value)
{
	this->Force = GetActorForwardVector() * Value * ThrustSpeed;

	PlaySoundOnNetwork(ThrusterAudioComponent, true);
	ThrusterAudioComponent->AdjustVolume(3,0,EAudioFaderCurve::Linear);
	
	// Name_None -> we don't use skeletal mesh with bones. Use force on the whole component
	// true -> accumulate force every new call of AddForce
	BoxComponent->AddForce(this->Force, NAME_None, true);
}

void APlayerSpaceShipPawn::MovePlayerEnhanced_Implementation(const FInputActionValue& Value)
{
	
	float MovementValue = Value.Get<float>();
	this->Force = GetActorForwardVector() * MovementValue * ThrustSpeed;
	PlaySoundOnNetwork(ThrusterAudioComponent, true);
	ThrusterAudioComponent->AdjustVolume(3, 0, EAudioFaderCurve::Linear);
    
	BoxComponent->AddForce(this->Force, NAME_None, true);
}

void APlayerSpaceShipPawn::RotatePlayer_Implementation(float Value)
{
	FVector Torque = FVector(0, 0, Value * RotationSpeed);
	BoxComponent->AddTorqueInDegrees(Torque, NAME_None, true);
}

void APlayerSpaceShipPawn::StartFire_Implementation()
{
	float FireRate = CurrentWeapon == 1 ? FireRateHomingMissile : FireRateStandardProjectile;
	GetWorld()->GetTimerManager().SetTimer(FireRateTimerHandle, this, &APlayerSpaceShipPawn::FireProjectile, FireRate, true, 0.0f);
}

void APlayerSpaceShipPawn::StopFire_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
}

void APlayerSpaceShipPawn::FireProjectileSound_Implementation()
{
 UGameplayStatics::PlaySoundAtLocation(this, LaserShotSound, GetActorLocation(), 0.3f);
}

void APlayerSpaceShipPawn::FireProjectile_Implementation()
{


	// true if ProjectileActorClass is set in BP_PlayerSpaceShipPawn
	if (ProjectileActorClass && HomingMissileActorClass)
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
			SpawnedProjectile = GetWorld()->SpawnActor<AActor>(HomingMissileActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
			FindClosestActor(MaxDistanceForSearchingActors, "Enemy");
			if (ClosestActor && SpawnedProjectile)
			{
				SetClosestActorForHomingMissile(SpawnedProjectile);
			}
		}
		
		if (SpawnedProjectile)
		{
			if (LaserShotSound)
			{
			   FireProjectileSound();
			}
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing Actor for Projectile!"));
	}
}


void APlayerSpaceShipPawn::HandleProjectileHit_Implementation(AActor* HitActor, AActor* ProjectileActor)
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

void APlayerSpaceShipPawn::FindClosestActor(float searchDistance, FName tag)
{
	TArray<AActor*> FoundActors;
	ClosestActor = nullptr;
	
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), tag, FoundActors);
	
	for (AActor* CurrentActor: FoundActors)
	{
		if(CurrentActor == this) continue;
		
		if (!ClosestActor)
		{
			const float DistanceClosestActor = (this->GetActorLocation() - CurrentActor->GetActorLocation()).Length();
			if (DistanceClosestActor < searchDistance)
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

float APlayerSpaceShipPawn::GetRadarRotationAngle(FName tag) {
	FindClosestActor(MaxDistanceForSearchingActorsForRadar, tag);

	if (!ClosestActor) return 0.0f;

    FVector Direction = ClosestActor->GetActorLocation() - this->GetActorLocation();
    Direction.Normalize();

    // Return Yaw (rotation angle)
    // Calculate the yaw angle in radians
    float YawAngleRad = FMath::Atan2(Direction.Y, Direction.X);

    // Convert radians to degrees
    float YawAngleDeg = FMath::RadiansToDegrees(YawAngleRad);

    // Create a rotator with the calculated yaw
    return YawAngleDeg;
}
