#include "World/WreckContainer.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/MineralMagnetComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

AWreckContainer::AWreckContainer()
{
	// Only ticks while the damage flash fades out.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlashMaterial(TEXT("/Game/GDTSurvivor/Materials/M_DamageFlashOverlay.M_DamageFlashOverlay"));
	DamageFlashMaterial = FlashMaterial.Object;

	// Spawned by the server (tiles), clients need to see it and its destruction.
	bReplicates = true;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->InitBoxExtent(FVector(50.0f));
	// Blocks ships; projectiles (OverlapAllDynamic) get an overlap and are destroyed
	// by HandleProjectileHit because of the "breakable" tag.
	Collision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCanEverAffectNavigation(true);
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	// Placeholder until there is a container mesh; same size as the collision box.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh->SetStaticMesh(CubeMesh.Object);

	Tags.Add(FName("breakable"));
}

void AWreckContainer::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
}

float AWreckContainer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
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

void AWreckContainer::MulticastPlayDamageFlash_Implementation()
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

void AWreckContainer::Tick(float DeltaSeconds)
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

void AWreckContainer::SetFlashIntensity(float Intensity)
{
	if (DamageFlashInstance)
	{
		DamageFlashInstance->SetScalarParameterValue(TEXT("Damage_Intensity"), Intensity);
	}
}

void AWreckContainer::Break()
{
	bBroken = true;
	MulticastPlayBreakEffects();
	SpawnLoot();
	Destroy();
}

void AWreckContainer::SpawnLoot()
{
	// Roll every entry first, so the pickups can be spread evenly around the center.
	TArray<TSubclassOf<AActor>> Drops;
	for (const FWreckLootEntry& Entry : Loot)
	{
		if (Entry.ActorClass && FMath::FRand() < Entry.Chance)
		{
			const int32 Count = FMath::RandRange(Entry.MinCount, FMath::Max(Entry.MinCount, Entry.MaxCount));
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Drops.Add(Entry.ActorClass);
			}
		}
	}
	if (Drops.IsEmpty())
	{
		return;
	}

	const FVector Center = GetActorLocation();
	const float AngleOffset = FMath::FRandRange(0.0f, 2.0f * PI);

	for (int32 Index = 0; Index < Drops.Num(); ++Index)
	{
		// Evenly around the center with some jitter, so they don't stack.
		const float Angle = AngleOffset + 2.0f * PI * Index / Drops.Num() + FMath::FRandRange(-0.3f, 0.3f);
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
		const FVector Location = Center + Direction * FMath::FRandRange(0.4f, 1.0f) * ScatterRadius;
		const FTransform SpawnTransform(FRotator(0.0, FMath::FRandRange(0.0f, 360.0f), 0.0), Location);

		// Deferred so the transform (incl. scale) is applied as given, see AEndlessTile::InitTile.
		if (AActor* Pickup = GetWorld()->SpawnActorDeferred<AActor>(Drops[Index], SpawnTransform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Pickup->FinishSpawning(SpawnTransform);

			// Minerals can drift; other pickups simply appear at their spot.
			if (UMineralMagnetComponent* Magnet = Pickup->FindComponentByClass<UMineralMagnetComponent>())
			{
				Magnet->Launch(Direction * LootLaunchSpeed * FMath::FRandRange(0.75f, 1.25f));
			}
		}
	}
}

void AWreckContainer::MulticastPlayBreakEffects_Implementation()
{
	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, GetActorLocation());
	}
	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
	}
}
