// Fill out your copyright notice in the Description page of Project Settings.


#include "MineBehaviour.h"
#include "Components/BoxComponent.h"

void UMineBehaviour::GDT_FollowActor(APawn* EnemyPawn, APawn* PlayerPawn, float Speed)
{
	FVector Direction = PlayerPawn->GetActorLocation() - EnemyPawn->GetActorLocation();
	const UBoxComponent* BoxComp = Cast<UBoxComponent>(EnemyPawn->GetComponentByClass(UBoxComponent::StaticClass()));
	UE_LOG(LogTemp, Warning, TEXT("Founded Box: %s"), *BoxComp->GetName());
	Direction.Normalize();
	UE_LOG(LogTemp, Warning, TEXT("Direction: %s"), *Direction.ToString());
	const FVector CurrentLocation = EnemyPawn->GetActorLocation();
	const FVector NewLocation = CurrentLocation + Direction * Speed;
	EnemyPawn->SetActorLocation(FVector(NewLocation.X, NewLocation.Y, CurrentLocation.Z), true);
}