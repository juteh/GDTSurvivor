#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MineralMagnetComponent.generated.h"

class APawn;

/**
 * Pulls the owning mineral to a player in range, like BP_XPPickup (same speeds, same
 * PickupRangeMultiplier stat of the player's UShipStatsComponent).
 * Collecting itself stays in the mineral Blueprint (its Box overlap).
 * Can also push the mineral away with a fading velocity (Launch), e.g. when an asteroid breaks.
 */
UCLASS(ClassGroup = (GDTSurvivor), meta = (BlueprintSpawnableComponent))
class GDTSURVIVOR_API UMineralMagnetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMineralMagnetComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Starts a drift that slows down over time (DriftDamping). Ignored once the magnet pulls.
	UFUNCTION(BlueprintCallable, Category = "Magnet")
	void Launch(const FVector& Velocity);

	// Base radius, multiplied with the player's pickup range upgrade.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnet", meta = (ClampMin = "0"))
	float MagnetRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnet", meta = (ClampMin = "0"))
	float MinAttractSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnet", meta = (ClampMin = "0"))
	float MaxAttractSpeed = 1400.0f;

	// How fast a launch drift slows down (per second). Higher = shorter drift.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnet", meta = (ClampMin = "0"))
	float DriftDamping = 2.5f;

protected:
	virtual void BeginPlay() override;

private:
	// Closest player pawn within its (upgraded) magnet radius, or null.
	APawn* FindPlayerInRange() const;

	static float GetPickupRangeMultiplier(const APawn* Pawn);

	void MoveTowardTarget(float DeltaTime);

	void UpdateDrift(float DeltaTime);

	TWeakObjectPtr<APawn> Target;

	FVector DriftVelocity = FVector::ZeroVector;
};
