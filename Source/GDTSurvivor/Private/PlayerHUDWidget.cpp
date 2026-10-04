#include "PlayerHUDWidget.h"

#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GDTSurvivor/PlayerSpaceShipPawn.h"
#include "ShipPlayerState.h"
#include "ShipStatsComponent.h"
#include "UpgradeDefinition.h"
#include "UpgradeEntryWidget.h"

#define LOCTEXT_NAMESPACE "PlayerHUD"

void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PawnChangedHandle = PlayerController->GetOnNewPawnNotifier().AddUObject(this, &UPlayerHUDWidget::HandlePawnChanged);
		HandlePawnChanged(PlayerController->GetPawn());
	}
}

void UPlayerHUDWidget::NativeDestruct()
{
	UnbindFromStats();
	UnbindFromPlayerState();

	if (APlayerSpaceShipPawn* Ship = BoundShip.Get())
	{
		Ship->OnWeaponChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleWeaponChanged);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->GetOnNewPawnNotifier().Remove(PawnChangedHandle);
	}

	Super::NativeDestruct();
}

void UPlayerHUDWidget::HandlePawnChanged(APawn* NewPawn)
{
	BindToStats(NewPawn ? NewPawn->FindComponentByClass<UShipStatsComponent>() : nullptr);

	if (APlayerSpaceShipPawn* OldShip = BoundShip.Get())
	{
		OldShip->OnWeaponChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleWeaponChanged);
	}
	BoundShip = Cast<APlayerSpaceShipPawn>(NewPawn);
	if (APlayerSpaceShipPawn* Ship = BoundShip.Get())
	{
		Ship->OnWeaponChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleWeaponChanged);
	}

	// The player state usually exists before the HUD; this also catches the case where it arrives later.
	if (!BoundPlayerState.IsValid())
	{
		BindToPlayerState(GetOwningPlayerState<AShipPlayerState>());
	}
}

void UPlayerHUDWidget::HandleWeaponChanged(const FString& WeaponName)
{
	OnSelectedWeaponChanged(WeaponName);
}

// Ship stats

void UPlayerHUDWidget::BindToStats(UShipStatsComponent* Stats)
{
	UnbindFromStats();
	if (!Stats)
	{
		return;
	}

	BoundStats = Stats;
	Stats->OnHealthChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleHealthChanged);
	Stats->OnShieldChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleShieldChanged);

	HandleHealthChanged(Stats->GetHealth(), Stats->GetMaxHealth());
	HandleShieldChanged(Stats->GetShield(), Stats->GetMaxShield());
}

void UPlayerHUDWidget::UnbindFromStats()
{
	if (UShipStatsComponent* Stats = BoundStats.Get())
	{
		Stats->OnHealthChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleHealthChanged);
		Stats->OnShieldChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleShieldChanged);
	}
	BoundStats.Reset();
}

void UPlayerHUDWidget::HandleHealthChanged(float Current, float Max)
{
	ShowResource(PROG_Health, CommonTextBlock_Health, Current, Max);
}

void UPlayerHUDWidget::HandleShieldChanged(float Current, float Max)
{
	ShowResource(PROG_Shield, CommonTextBlock_Shield, Current, Max);
}

void UPlayerHUDWidget::ShowResource(UProgressBar* Bar, UTextBlock* Text, float Current, float Max)
{
	Bar->SetPercent(Max > 0.f ? Current / Max : 0.f);
	Text->SetText(FText::Format(LOCTEXT("Resource", "{0}/{1}"),
		FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
}

// Player state

void UPlayerHUDWidget::BindToPlayerState(AShipPlayerState* PlayerState)
{
	UnbindFromPlayerState();
	if (!PlayerState)
	{
		return;
	}

	BoundPlayerState = PlayerState;
	PlayerState->OnScoreChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleScoreChanged);
	PlayerState->OnExperienceChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleExperienceChanged);
	PlayerState->OnUpgradesChanged.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleUpgradesChanged);

	HandleScoreChanged(PlayerState->GetScoreAsInt());
	HandleExperienceChanged(PlayerState->GetExperience(), PlayerState->GetExperiencePerLevel(), PlayerState->GetPlayerLevel());
	HandleUpgradesChanged();
}

void UPlayerHUDWidget::UnbindFromPlayerState()
{
	if (AShipPlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnScoreChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleScoreChanged);
		PlayerState->OnExperienceChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleExperienceChanged);
		PlayerState->OnUpgradesChanged.RemoveDynamic(this, &UPlayerHUDWidget::HandleUpgradesChanged);
	}
	BoundPlayerState.Reset();
}

void UPlayerHUDWidget::HandleScoreChanged(int32 NewScore)
{
	CommonTextBlock_Score->SetText(FText::AsNumber(NewScore));
}

void UPlayerHUDWidget::HandleExperienceChanged(int32 Experience, int32 ExperiencePerLevel, int32 Level)
{
	CommonTextBlock_LVL->SetText(FText::Format(LOCTEXT("Level", "LVL: {0}"), Level));
	CommonTextBlock_EXP->SetText(FText::Format(LOCTEXT("Experience", "{0}/{1}"), Experience, ExperiencePerLevel));
	ProgressBar_Experience->SetPercent(ExperiencePerLevel > 0 ? static_cast<float>(Experience) / ExperiencePerLevel : 0.f);
}

void UPlayerHUDWidget::HandleUpgradesChanged()
{
	const AShipPlayerState* PlayerState = BoundPlayerState.Get();
	if (!PlayerState || !UpgradeEntryClass)
	{
		return;
	}

	VerticalBox_UpgradeEntries->ClearChildren();

	// In the order the upgrades were first chosen.
	for (const FUpgradeStack& Stack : PlayerState->GetUpgradeStacks())
	{
		if (!Stack.Upgrade || Stack.Stacks <= 0)
		{
			continue;
		}

		UUpgradeEntryWidget* Entry = CreateWidget<UUpgradeEntryWidget>(GetOwningPlayer(), UpgradeEntryClass);
		Entry->SetEntry(Stack.Upgrade->DisplayName, Stack.Stacks);
		VerticalBox_UpgradeEntries->AddChild(Entry);
	}
}

#undef LOCTEXT_NAMESPACE
