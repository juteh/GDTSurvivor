#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MineBehaviour.generated.h"

UCLASS()
class GDTSURVIVOR_API UMineBehaviour : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
		// Functions in BlueprintFunctionLibrary that use Actors/Pawns need a WorldContext
		UFUNCTION(BlueprintCallable, Category = "Utility", BlueprintPure = false, meta=(WorldContext="WorldContextObject"))
		static void GDT_FollowActor(APawn* EnemyPawn, APawn* PlayerPawn, float Speed);
};
