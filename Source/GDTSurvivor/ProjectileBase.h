

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

UENUM(BlueprintType)
enum class EProjectileOrigin :uint8
{
	AI,
	PLAYER
};

UCLASS()
class GDTSURVIVOR_API AProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectileBase();

	UPROPERTY(BlueprintReadWrite, Category="Stats")
	APlayerController *OriginPlayerController;
	
	UPROPERTY(BlueprintReadWrite, Category="Stats")
	EProjectileOrigin OriginType = EProjectileOrigin::AI;

	// Damage dealt on hit. Base value is set per Blueprint class default (e.g. BP_Projectile);
	// the spawning pawn may add a flat bonus on top after spawning (see APlayerSpaceShipPawn::DamageBonus).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float ProjectileDamage = 0.f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
