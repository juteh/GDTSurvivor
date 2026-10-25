#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GDTMenuPlayerController.generated.h"

class UUserWidget;

/**
 * Player controller of the menu maps (main menu, ship menu). Opens the menu of its map when it begins play:
 * the map's GameMode picks the controller, the controller's Blueprint subclass picks the widget
 * (PC_Player → WBP_MainMenuStack, PC_ShipMenu → WBP_SpaceshipMenuStack).
 * See Docs/Architecture.md, section 2.
 */
UCLASS()
class GDTSURVIVOR_API AGDTMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	// Root widget of this map's menu, usually a CommonUI menu stack.
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UUserWidget> MenuWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> MenuWidget;
};
