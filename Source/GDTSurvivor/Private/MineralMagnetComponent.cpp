#include "MineralMagnetComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
	// Searching for players is only needed a few times per second while idle.
	constexpr float IdleTickInterval = 0.1f;

	// Below this speed a drift counts as finished.
	constexpr float MinDriftSpeed = 5.0f;
}

UMineralMagnetComponent::UMineralMagnetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = IdleTickInterval;
}

void UMineralMagnetComponent::BeginPlay()
{
	Super::BeginPlay();
	// Spread the idle checks of many minerals over different frames.
	SetComponentTickInterval(IdleTickInterval * FMath::FRandRange(0.8f, 1.2f));
}

void UMineralMagnetComponent::Launch(const FVector& Velocity)
{
	DriftVelocity = FVector(Velocity.X, Velocity.Y, 0.0);
	SetComponentTickInterval(0.0f);
}

void UMineralMagnetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Target.IsValid())
	{
		if (APawn* Player = FindPlayerInRange())
		{
			Target = Player;
			DriftVelocity = FVector::ZeroVector;
			// Smooth movement from now on.
			SetComponentTickInterval(0.0f);
		}
	}

	if (Target.IsValid())
	{
		MoveTowardTarget(DeltaTime);
	}
	else if (!DriftVelocity.IsZero())
	{
		UpdateDrift(DeltaTime);
	}
}

APawn* UMineralMagnetComponent::FindPlayerInRange() const
{
	const FVector Location = GetOwner()->GetActorLocation();
	APawn* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		const float Radius = MagnetRadius * GetPickupRangeMultiplier(Pawn);
		const float DistSq = FVector::DistSquared(Location, Pawn->GetActorLocation());
		if (DistSq <= Radius * Radius && DistSq < ClosestDistSq)
		{
			Closest = Pawn;
			ClosestDistSq = DistSq;
		}
	}
	return Closest;
}

float UMineralMagnetComponent::GetPickupRangeMultiplier(const APawn* Pawn)
{
	// GetPickupRangeMultiplier lives in BP_PlayerStateSpaceShip (Blueprint), so call it via reflection.
	APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
	UFunction* Function = PlayerState ? PlayerState->FindFunction(TEXT("GetPickupRangeMultiplier")) : nullptr;
	if (!Function)
	{
		return 1.0f;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(Function->ParmsSize));
	FMemory::Memzero(Params, Function->ParmsSize);
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Params);
	}

	PlayerState->ProcessEvent(Function, Params);

	float Multiplier = 1.0f;
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(*It))
			{
				Multiplier = DoubleProp->GetPropertyValue_InContainer(Params);
			}
			else if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(*It))
			{
				Multiplier = FloatProp->GetPropertyValue_InContainer(Params);
			}
		}
		It->DestroyValue_InContainer(Params);
	}
	return Multiplier;
}

void UMineralMagnetComponent::MoveTowardTarget(float DeltaTime)
{
	// Same formula as BP_XPPickup.MoveTowardTarget: faster the closer it gets.
	AActor* Owner = GetOwner();
	const FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	const float Dist = ToTarget.Size();
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Closeness = FMath::Clamp(1.0f - Dist / FMath::Max(MagnetRadius, 1.0f), 0.0f, 1.0f);
	const float Speed = FMath::Lerp(MinAttractSpeed, MaxAttractSpeed, Closeness);
	const float Step = FMath::Min(Speed * DeltaTime, Dist);
	Owner->SetActorLocation(Owner->GetActorLocation() + ToTarget / Dist * Step);
}

void UMineralMagnetComponent::UpdateDrift(float DeltaTime)
{
	AActor* Owner = GetOwner();
	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + DriftVelocity * DeltaTime;

	// Stop at walls/blocks instead of drifting into them.
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		DriftVelocity = FVector::ZeroVector;
	}
	else
	{
		Owner->SetActorLocation(End);
		DriftVelocity *= FMath::Exp(-DriftDamping * DeltaTime);
	}

	if (DriftVelocity.SizeSquared() < MinDriftSpeed * MinDriftSpeed)
	{
		DriftVelocity = FVector::ZeroVector;
		SetComponentTickInterval(IdleTickInterval);
	}
}
