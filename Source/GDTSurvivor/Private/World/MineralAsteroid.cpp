#include "World/MineralAsteroid.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/MineralMagnetComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

AMineralAsteroid::AMineralAsteroid()
{
	// Only ticks while the damage flash fades out.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlashMaterial(TEXT("/Game/GDTSurvivor/Materials/M_DamageFlashOverlay.M_DamageFlashOverlay"));
	DamageFlashMaterial = FlashMaterial.Object;

	// Spawned by the server (tiles), clients need to see it and its destruction.
	bReplicates = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	// Authored for actor scale 1 (mineral meshes are ~100uu); the size comes from the actor scale.
	Collision->InitSphereRadius(50.0f);
	// Blocks ships, projectiles (OverlapAllDynamic) get an overlap and are destroyed
	// by HandleProjectileHit because of the "asteroid" tag.
	Collision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCanEverAffectNavigation(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	Rotation = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Rotation"));
	// Only the mesh spins, so the collision sphere stays put.
	Rotation->SetUpdatedComponent(Mesh);
	Rotation->RotationRate = FRotator::ZeroRotator;

	Tags.Add(FName("asteroid"));
}

void AMineralAsteroid::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	Rotation->RotationRate = FRotator(
		FMath::FRandRange(-MaxSpinSpeed, MaxSpinSpeed),
		FMath::FRandRange(-MaxSpinSpeed, MaxSpinSpeed),
		FMath::FRandRange(-MaxSpinSpeed, MaxSpinSpeed));
}

float AMineralAsteroid::GetFootprintRadius() const
{
	return Collision->GetScaledSphereRadius();
}

void AMineralAsteroid::SetSizeStats(float InMaxHealth, int32 InMineralCount)
{
	MaxHealth = InMaxHealth;
	MineralCount = InMineralCount;
}

float AMineralAsteroid::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (!HasAuthority() || bBroken || ActualDamage <= 0.0f)
	{
		return ActualDamage;
	}

	Health -= ActualDamage;
	if (Health <= 0.0f)
	{
		Break();
	}
	else
	{
		MulticastPlayDamageFlash();
	}
	return ActualDamage;
}

void AMineralAsteroid::MulticastPlayDamageFlash_Implementation()
{
	if (!DamageFlashMaterial)
	{
		return;
	}
	if (!DamageFlashInstance)
	{
		DamageFlashInstance = UMaterialInstanceDynamic::Create(DamageFlashMaterial, this);
		Mesh->SetOverlayMaterial(DamageFlashInstance);
	}

	FlashTimeRemaining = DamageFlashDuration;
	SetFlashIntensity(DamageFlashIntensity);
	SetActorTickEnabled(true);
}

void AMineralAsteroid::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FlashTimeRemaining -= DeltaSeconds;
	if (FlashTimeRemaining <= 0.0f)
	{
		SetFlashIntensity(0.0f);
		SetActorTickEnabled(false);
		return;
	}
	SetFlashIntensity(DamageFlashIntensity * FlashTimeRemaining / DamageFlashDuration);
}

void AMineralAsteroid::SetFlashIntensity(float Intensity)
{
	if (DamageFlashInstance)
	{
		DamageFlashInstance->SetScalarParameterValue(TEXT("Damage_Intensity"), Intensity);
	}
}

void AMineralAsteroid::Break()
{
	bBroken = true;
	MulticastPlayBreakEffects();

	if (MineralClass && MineralCount > 0)
	{
		const FVector Center = GetActorLocation();
		const float ScatterRadius = GetFootprintRadius() * ScatterRadiusFactor;
		const float AngleOffset = FMath::FRandRange(0.0f, 2.0f * PI);

		for (int32 Index = 0; Index < MineralCount; ++Index)
		{
			// Evenly around the center with some jitter, so they don't stack.
			const float Angle = AngleOffset + 2.0f * PI * Index / MineralCount + FMath::FRandRange(-0.3f, 0.3f);
			const float Distance = FMath::FRandRange(0.4f, 1.0f) * ScatterRadius;
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
			const FVector Location = Center + Direction * Distance;
			const FTransform SpawnTransform(FRotator(0.0, FMath::FRandRange(0.0f, 360.0f), 0.0), Location);

			// Deferred so the transform (incl. scale) is applied as given, see AEndlessTile::InitTile.
			if (AActor* Mineral = GetWorld()->SpawnActorDeferred<AActor>(MineralClass, SpawnTransform,
				nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				Mineral->FinishSpawning(SpawnTransform);

				// Push away from the center (minerals get the component in BP_Mineral_Base).
				if (UMineralMagnetComponent* Magnet = Mineral->FindComponentByClass<UMineralMagnetComponent>())
				{
					Magnet->Launch(Direction * MineralLaunchSpeed * FMath::FRandRange(0.75f, 1.25f));
				}
			}
		}
	}

	Destroy();
}

void AMineralAsteroid::MulticastPlayBreakEffects_Implementation()
{
	if (BreakEffect)
	{
		// Same effect and size as an enemy explosion; bigger asteroids get more of them.
		const int32 NumExplosions = FMath::Max(1, FMath::RoundToInt(GetActorScale3D().X * ExplosionsPerScale));
		const float Spread = GetFootprintRadius() * ExplosionSpread;
		for (int32 Index = 0; Index < NumExplosions; ++Index)
		{
			const FVector Offset = Index == 0
				? FVector::ZeroVector
				: FVector(FMath::FRandRange(-Spread, Spread), FMath::FRandRange(-Spread, Spread), 0.0);
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, GetActorLocation() + Offset);
		}
	}
	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
	}
}
