#include "World/EndlessTileAuthoring.h"

#include "Components/BoxComponent.h"
#include "World/EndlessTileData.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "FileHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/PackageName.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

AEndlessTileAuthoring::AEndlessTileAuthoring()
{
	PrimaryActorTick.bCanEverTick = false;

	TileBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("TileBounds"));
	TileBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TileBounds->SetCanEverAffectNavigation(false);
	TileBounds->ShapeColor = FColor::Green;
	TileBounds->SetLineThickness(30.0f);
	RootComponent = TileBounds;

	FreeEdgeBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("FreeEdgeBounds"));
	FreeEdgeBounds->SetupAttachment(TileBounds);
	FreeEdgeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FreeEdgeBounds->SetCanEverAffectNavigation(false);
	FreeEdgeBounds->ShapeColor = FColor::Orange;
	FreeEdgeBounds->SetLineThickness(15.0f);

	SetActorHiddenInGame(true);
}

#if WITH_EDITOR

void AEndlessTileAuthoring::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float Half = 0.5f * TileSize;
	TileBounds->SetBoxExtent(FVector(Half, Half, 400.0f));
	const float Inner = FMath::Max(Half - EdgeMargin, 0.0f);
	FreeEdgeBounds->SetBoxExtent(FVector(Inner, Inner, 400.0f));
}

UEndlessTileData* AEndlessTileAuthoring::FindOrCreateDataAsset()
{
	const FString MapPackage = GetLevel()->GetOutermost()->GetName();
	const FString AssetName = TEXT("DA_") + FPackageName::GetShortName(MapPackage);
	const FString PackageName = FPackageName::GetLongPackagePath(MapPackage) / AssetName;

	UEndlessTileData* Data = LoadObject<UEndlessTileData>(nullptr, *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Data)
	{
		UPackage* Package = CreatePackage(*PackageName);
		Data = NewObject<UEndlessTileData>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Data);
	}

	Modify();
	TargetData = Data;
	return Data;
}

void AEndlessTileAuthoring::Bake()
{
	ULevel* Level = GetLevel();
	if (!Level)
	{
		return;
	}

	UEndlessTileData* Data = TargetData ? TargetData.Get() : FindOrCreateDataAsset();
	Data->Modify();

	const FString MapPackage = Level->GetOutermost()->GetName();
	Data->TileName = TileName.IsEmpty() ? FPackageName::GetShortName(MapPackage) : TileName;
	Data->bAllowRotation = bAllowRotation;
	Data->Weight = Weight;
	Data->bAllowMineralAsteroids = bAllowMineralAsteroids;
	Data->SourceMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(MapPackage + TEXT(".") + FPackageName::GetShortName(MapPackage)));
	Data->MeshGroups.Reset();
	Data->Actors.Reset();

	// Everything is stored relative to this actor (scale ignored).
	const FTransform Origin(GetActorRotation(), GetActorLocation());
	const float Half = 0.5f * TileSize;
	TArray<FString> OutOfBounds;
	int32 NumMeshes = 0;

	for (AActor* Actor : Level->Actors)
	{
		if (!Actor || Actor == this || Actor->IsEditorOnly() || Actor->ActorHasTag(TEXT("NoBake")))
		{
			continue;
		}

		const FTransform Relative = Actor->GetActorTransform().GetRelativeTransform(Origin);
		bool bBaked = false;

		if (const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
		{
			const UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();
			if (MeshComponent && MeshComponent->GetStaticMesh())
			{
				TArray<TObjectPtr<UMaterialInterface>> Materials;
				for (int32 Index = 0; Index < MeshComponent->GetNumMaterials(); ++Index)
				{
					Materials.Add(MeshComponent->GetMaterial(Index));
				}
				const FName Profile = MeshComponent->GetCollisionProfileName();

				FEndlessTileMeshGroup* Group = Data->MeshGroups.FindByPredicate([&](const FEndlessTileMeshGroup& Existing)
				{
					return Existing.Mesh == MeshComponent->GetStaticMesh() && Existing.Materials == Materials && Existing.CollisionProfile == Profile;
				});
				if (!Group)
				{
					Group = &Data->MeshGroups.AddDefaulted_GetRef();
					Group->Mesh = MeshComponent->GetStaticMesh();
					Group->Materials = Materials;
					Group->CollisionProfile = Profile;
				}
				Group->Instances.Add(MeshComponent->GetComponentTransform().GetRelativeTransform(Origin));
				++NumMeshes;
				bBaked = true;
			}
		}
		else if (Cast<UBlueprintGeneratedClass>(Actor->GetClass()))
		{
			// Gameplay actors made in Blueprint: minerals, pickups, ...
			FEndlessTileActorEntry& Entry = Data->Actors.AddDefaulted_GetRef();
			Entry.ActorClass = Actor->GetClass();
			Entry.Transform = Relative;
			bBaked = true;
		}

		if (bBaked && (FMath::Abs(Relative.GetLocation().X) > Half || FMath::Abs(Relative.GetLocation().Y) > Half))
		{
			OutOfBounds.Add(Actor->GetActorLabel());
		}
	}

	Data->MarkPackageDirty();
	UEditorLoadingAndSavingUtils::SavePackages({ Data->GetOutermost() }, false);

	FString Message = FString::Printf(TEXT("%s gebacken: %d Meshes, %d Actors"), *Data->TileName, NumMeshes, Data->Actors.Num());
	if (OutOfBounds.Num() > 0)
	{
		Message += TEXT("\nAusserhalb der Kachel: ") + FString::Join(OutOfBounds, TEXT(", "));
	}
	UE_LOG(LogTemp, Display, TEXT("EndlessTileAuthoring: %s"), *Message);

	FNotificationInfo Info(FText::FromString(Message));
	Info.ExpireDuration = OutOfBounds.Num() > 0 ? 8.0f : 4.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

#endif
