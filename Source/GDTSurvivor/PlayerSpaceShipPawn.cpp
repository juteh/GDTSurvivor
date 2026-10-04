	// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpaceShipPawn.h"

#include "Engine/OverlapResult.h"
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
#include "ProjectileBase.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "Components/Widget.h"
#include "EngineUtils.h"
#include "MineralAsteroid.h"
#include "ShipStatsComponent.h"

// Sets default values
APlayerSpaceShipPawn::APlayerSpaceShipPawn()
{
	this->bReplicates=true;
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// set BoxCollider for RootComponent of blueprint
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollisionComponent"));
	RootComponent = BoxComponent;
	// deal ram damage on physical collision with an enemy ship (in addition to whatever the Blueprint's own OnComponentHit binding does)
	BoxComponent->OnComponentHit.AddDynamic(this, &APlayerSpaceShipPawn::OnShipCollision);

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
	ThrusterAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	ThrusterAudioComponent->SetupAttachment(RootComponent);
	ThrusterAudioComponent->bAutoActivate = true;
	ThrusterAudioComponent->SetIsReplicated(true);
	
	// create spawn point for projectile
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(SpaceshipMesh);

	ShipStats = CreateDefaultSubobject<UShipStatsComponent>(TEXT("ShipStats"));
}

UNiagaraComponent* APlayerSpaceShipPawn::CreateThrusterFX(const FVector& Location, const FRotator& Rotation, const FVector& Scale) const
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
		ThrusterAudioComponent->SetIsReplicated(true);
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
		
	ENetMode NetMode = GetNetMode();

	if((NetMode == NM_DedicatedServer || NetMode == NM_ListenServer) && shouldPlaySound) {
		ThrusterAudioComponent->AdjustVolume(2,CurrentThrusterVolume,EAudioFaderCurve::Linear);
	}

	
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
	const float BaseFireRate = CurrentWeapon == 1 ? FireRateHomingMissile : FireRateStandardProjectile;
	const float FireRate = BaseFireRate * ShipStats->GetStat(EShipStat::FireIntervalMultiplier);
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
		AProjectileBase* SpawnedProjectile;
		if (CurrentWeapon == 0)
		{
			SpawnedProjectile = GetWorld()->SpawnActor<AProjectileBase>(ProjectileActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
		} else
		{
			SpawnedProjectile = GetWorld()->SpawnActor<AProjectileBase>(HomingMissileActorClass, SpawnLocation, SpawnRotation, SpawnParameters);
			SetClosestActorForHomingMissile(SpawnedProjectile);
		}
		
		if (SpawnedProjectile)
		{

			SpawnedProjectile->OriginPlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			SpawnedProjectile->OriginType = EProjectileOrigin::PLAYER;
			SpawnedProjectile->ProjectileDamage += ShipStats->GetStat(EShipStat::DamageBonus);
			if (LaserShotSound)
			{
			   FireProjectileSound();
			}
		}
	} else
	{
		// Missing actor class
	}
}


void APlayerSpaceShipPawn::OnShipCollision(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only apply damage on the server - clients may also receive this Hit event locally,
	// but TakeDamage/health changes must stay authoritative.
	if (!HasAuthority() || !OtherActor)
	{
		return;
	}

	if (!OtherActor->ActorHasTag(TEXT("enemy")))
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (const float* LastTime = LastRamDamageTimeByActor.Find(OtherActor))
	{
		if (Now - *LastTime < RamDamageCooldown)
		{
			return;
		}
	}
	LastRamDamageTimeByActor.Add(OtherActor, Now);

	UGameplayStatics::ApplyDamage(this, RamDamage, nullptr, OtherActor, RamDamageTypeClass);
	UGameplayStatics::ApplyDamage(OtherActor, RamDamage, GetController(), this, RamDamageTypeClass);
}

void APlayerSpaceShipPawn::HandleProjectileHit_Implementation(AActor* HitActor, AActor* ProjectileActor, UActorComponent* HitComponent)
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
				ProjectileActor->Destroy();
			}
		}
		else if (HitActor->ActorHasTag("level") || HitActor->ActorHasTag("enemy") || HitActor->ActorHasTag("asteroid"))
		{
			ProjectileActor->Destroy();
		}
	} else
	{
		// Missing HitActor or ProjectileActor
	}
}

AActor* APlayerSpaceShipPawn::FindClosestActor(const float MaxDistanceForSearching, const FName Tag)
{
	AActor* ClosestActor = nullptr;

	// Use squared distance to avoid expensive sqrt calculations
	float ClosestDistanceSquared = MaxDistanceForSearching * MaxDistanceForSearching;
	const FVector MyLocation = GetActorLocation();

	// Use Sphere Overlap instead of iterating all actors - O(log n) vs O(n)
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		MyLocation,
		FQuat::Identity,
		ECC_Pawn,  // Collision channel for pawns
		FCollisionShape::MakeSphere(MaxDistanceForSearching),
		QueryParams
	);

	if (bHit)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			if (Actor && Actor->ActorHasTag(Tag))
			{
				const float DistanceSquared = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
				if (DistanceSquared < ClosestDistanceSquared)
				{
					ClosestDistanceSquared = DistanceSquared;
					ClosestActor = Actor;
				}
			}
		}
	}
	return ClosestActor;
}

float APlayerSpaceShipPawn::GetRadarRotationAngle(const FName Tag)
{
      const AActor* CurrentClosestActor = FindClosestActor(MaxDistanceForSearchingActorsForRadar, Tag);
      if (!CurrentClosestActor || !IsValid(CurrentClosestActor))
      {
          return 0.0f;
      }

      return GetRadarAngleToLocation(CurrentClosestActor->GetActorLocation());
}

float APlayerSpaceShipPawn::GetRadarAngleToLocation(const FVector& TargetLocation) const
{
      // Arrow rotation for a target, based on the world-space direction (see GetRadarScreenDirection).
      return GetRadarWorldDirection(TargetLocation) - RadarArrowTextureDirection;
}

float APlayerSpaceShipPawn::GetRadarWorldDirection(const FVector& TargetLocation) const
{
      // The camera looks along +X without inheriting the ship's yaw:
      // world +X is screen-up, world +Y is screen-right.
      // (Earlier versions started at an offset that was rotated with the ship, which made
      // the compass swing whenever the ship turned.)
      const FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal2D();
      return FMath::RadiansToDegrees(FMath::Atan2(-Direction.X, Direction.Y));
}

float APlayerSpaceShipPawn::GetRadarScreenDirection(const FVector& TargetLocation) const
{
	// From where the ship is drawn to where the target is drawn, so the arrow shows the player's
	// direction to the target as seen on screen, regardless of camera angle or ship rotation.
	const APlayerController* PC = GetController<APlayerController>();
	FVector2D TargetScreen;
	FVector2D PlayerScreen;
	if (PC
		&& PC->ProjectWorldLocationToScreen(TargetLocation, TargetScreen, true)
		&& PC->ProjectWorldLocationToScreen(GetActorLocation(), PlayerScreen, true)
		&& !(TargetScreen - PlayerScreen).IsNearlyZero())
	{
		const FVector2D Direction = TargetScreen - PlayerScreen;
		return FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	}
	return GetRadarWorldDirection(TargetLocation);
}

void APlayerSpaceShipPawn::PlaceRadarArrow(UWidget* Arrow, float ScreenDirection) const
{
	// Like a compass needle on a ring: the arrow sits on a circle around its designer position,
	// on the side of the target, and points outwards.
	const float Radians = FMath::DegreesToRadians(ScreenDirection);
	Arrow->SetRenderTranslation(FVector2D(FMath::Cos(Radians), FMath::Sin(Radians)) * RadarRingRadius);
	Arrow->SetRenderTransformAngle(ScreenDirection - RadarArrowTextureDirection);
}

void APlayerSpaceShipPawn::UpdateRadarArrows(UWidget* AsteroidArrow, UWidget* DropzoneArrow)
{
	auto ApplyColor = [](UWidget* Arrow, const FLinearColor& Color)
	{
		UImage* Image = Cast<UImage>(Arrow);
		if (Image && !Image->GetColorAndOpacity().Equals(Color))
		{
			Image->SetColorAndOpacity(Color);
		}
	};
	ApplyColor(AsteroidArrow, AsteroidArrowColor);
	ApplyColor(DropzoneArrow, DropzoneArrowColor);

	if (AsteroidArrow)
	{
		const AActor* Asteroid = FindClosestMineralAsteroid();
		AsteroidArrow->SetVisibility(Asteroid ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (Asteroid)
		{
			PlaceRadarArrow(AsteroidArrow, GetRadarScreenDirection(Asteroid->GetActorLocation()));
		}
	}

	if (DropzoneArrow)
	{
		// Without a dropzone (start tile currently recycled) point to the start tile center,
		// which is always the world origin.
		const AActor* Dropzone = FindDropzone();
		const FVector Target = Dropzone ? Dropzone->GetActorLocation() : FVector::ZeroVector;
		PlaceRadarArrow(DropzoneArrow, GetRadarScreenDirection(Target));
	}
}

AActor* APlayerSpaceShipPawn::FindClosestMineralAsteroid() const
{
	// Only a few dozen asteroids exist (spawned by the endless tiles), so iterating them is cheap.
	AActor* Closest = nullptr;
	double ClosestDistSq = TNumericLimits<double>::Max();
	const FVector MyLocation = GetActorLocation();
	for (TActorIterator<AMineralAsteroid> It(GetWorld()); It; ++It)
	{
		const double DistSq = FVector::DistSquared(MyLocation, It->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = *It;
		}
	}
	return Closest;
}

AActor* APlayerSpaceShipPawn::FindDropzone()
{
	if (CachedDropzone.IsValid())
	{
		return CachedDropzone.Get();
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextDropzoneSearchTime)
	{
		return nullptr;
	}
	NextDropzoneSearchTime = Now + 1.0;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(this, DropzoneTag, Found);
	CachedDropzone = Found.Num() > 0 ? Found[0] : nullptr;
	return CachedDropzone.Get();
}

AActor* APlayerSpaceShipPawn::FindClosestTarget(const FName Tag)
{
	return FindClosestActor(MaxDistanceForSearchingActors, Tag);
}
