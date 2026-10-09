#pragma once

#include "CoreMinimal.h"
#include "Components/ProgressBar.h"
#include "Fonts/SlateFontInfo.h"
#include "SegmentedBar.generated.h"

class SSegmentedBar;

UENUM(BlueprintType)
enum class ESegmentShape : uint8
{
	// Rectangular segments inside a frame with cut corners.
	Block,
	// Rounded segments inside a rounded frame.
	Pill
};

/**
 * Sci-fi resource bar for the HUD: a thin frame with an optional label in its top edge and a row of
 * segments. Full segments use the fill color, empty ones a faint version of it, and the segment that
 * holds the current value fades in between.
 *
 * It is a UProgressBar, so code that fills a progress bar (SetPercent, SetFillColorAndOpacity) works
 * unchanged. Turn an existing progress bar into one with "Replace With > Segmented Bar" in the designer.
 * The progress bar style properties are ignored.
 */
UCLASS(meta = (DisplayName = "Segmented Bar"))
class GDTSURVIVOR_API USegmentedBar : public UProgressBar
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Segments", meta = (ClampMin = "1", ClampMax = "100"))
	int32 SegmentCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Segments")
	ESegmentShape SegmentShape = ESegmentShape::Block;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Segments", meta = (ClampMin = "0"))
	float SegmentGap = 3.f;

	// Opacity of the fill color for empty segments.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Segments", meta = (ClampMin = "0", ClampMax = "1"))
	float EmptyOpacity = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame")
	FLinearColor FrameColor = FLinearColor(0.55f, 0.8f, 1.f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame", meta = (ClampMin = "0"))
	float FrameThickness = 1.f;

	// Space between the frame and the segments.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame")
	FMargin FramePadding = FMargin(4.f);

	// Size of the cut corners (top left, bottom right). Block shape only.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame", meta = (ClampMin = "0"))
	float CornerCut = 6.f;

	// Height of the frame. The label adds half its own height above it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frame", meta = (ClampMin = "4"))
	float BarHeight = 18.f;

	// Written into the top edge of the frame. Empty = no label.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Label")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Label")
	FSlateFontInfo LabelFont;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Label")
	FLinearColor LabelColor = FLinearColor(0.55f, 0.8f, 1.f, 1.f);

	// Minimum space between the bottom of the label and the segments. Pushes the segments down if
	// the frame padding alone would let the label touch them.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Label", meta = (ClampMin = "0"))
	float LabelSpacing = 4.f;

	// Changes the text in the top edge of the frame at runtime (e.g. "LEVEL 5").
	UFUNCTION(BlueprintCallable, Category = "Label")
	void SetLabel(const FText& InLabel);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SSegmentedBar> MySegmentedBar;
};
