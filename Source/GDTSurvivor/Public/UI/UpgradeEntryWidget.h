#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "UpgradeEntryWidget.generated.h"

class UTextBlock;

// Parent class of WBP_UpgradeEntry: one line of the chosen level-up upgrades in the HUD.
UCLASS(Abstract)
class GDTSURVIVOR_API UUpgradeEntryWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void SetEntry(const FText& Name, int32 Stacks);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CommonTextBlock_UpgradeText;
};
