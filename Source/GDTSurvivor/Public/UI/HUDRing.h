#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "HUDRing.generated.h"

class SHUDRing;

/**
 * Minimal ring for the HUD in the style of the compass ring (T_CompassRing), e.g. around the selected
 * weapon icon: one evenly thick circle with a thin dark outline on both edges, so it stays readable on
 * bright backgrounds.
 * Put it into an Overlay together with the content it should frame (the content on top, centered).
 */
UCLASS(meta = (DisplayName = "HUD Ring"))
class GDTSURVIVOR_API UHUDRing : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	FLinearColor Color = FLinearColor(0.9f, 0.95f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (ClampMin = "8"))
	float Diameter = 72.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (ClampMin = "1"))
	float Thickness = 3.f;

	// Alpha 0 = no outline.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
	FLinearColor OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline", meta = (ClampMin = "0"))
	float OutlineThickness = 1.f;

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SHUDRing> MyRing;
};
