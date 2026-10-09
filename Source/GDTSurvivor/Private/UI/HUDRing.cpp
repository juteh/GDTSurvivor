#include "UI/HUDRing.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "HUDRing"

namespace
{
	// Copy of the UHUDRing properties, so the Slate widget does not read the UObject while painting.
	struct FHUDRingAppearance
	{
		FLinearColor Color = FLinearColor::White;
		float Diameter = 72.f;
		float Thickness = 3.f;
		FLinearColor OutlineColor = FLinearColor::Black;
		float OutlineThickness = 1.f;
	};

	TArray<FVector2f> MakeCircle(const FVector2f& Center, float Radius)
	{
		// About one point every 6 degrees keeps the circle round at HUD sizes.
		constexpr int32 Steps = 60;
		TArray<FVector2f> Points;
		Points.Reserve(Steps + 1);
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Radians = 2.f * PI * Step / Steps;
			Points.Add(Center + Radius * FVector2f(FMath::Cos(Radians), FMath::Sin(Radians)));
		}
		return Points;
	}
}

class SHUDRing : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SHUDRing) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
	}

	void SetAppearance(const FHUDRingAppearance& InAppearance)
	{
		Appearance = InAppearance;
		Invalidate(EInvalidateWidgetReason::Layout);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(Appearance.Diameter, Appearance.Diameter);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FVector2f Size = AllottedGeometry.GetLocalSize();
		const FVector2f Center = Size * 0.5f;
		const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
		const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

		const float Outline = Appearance.OutlineColor.A > 0.f ? Appearance.OutlineThickness : 0.f;
		const float Radius = FMath::Min(Size.X, Size.Y) * 0.5f - Outline - Appearance.Thickness * 0.5f;
		if (Radius <= 0.f)
		{
			return LayerId;
		}

		// The outline is one wider line below the ring, so it shows on the inner and outer edge.
		if (Outline > 0.f)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, PaintGeometry, MakeCircle(Center, Radius),
				DrawEffects, Appearance.OutlineColor * Tint, true, Appearance.Thickness + 2.f * Outline);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, PaintGeometry, MakeCircle(Center, Radius),
			DrawEffects, Appearance.Color * Tint, true, Appearance.Thickness);

		return LayerId + 2;
	}

private:
	FHUDRingAppearance Appearance;
};

TSharedRef<SWidget> UHUDRing::RebuildWidget()
{
	MyRing = SNew(SHUDRing);
	return MyRing.ToSharedRef();
}

void UHUDRing::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!MyRing.IsValid())
	{
		return;
	}

	FHUDRingAppearance Appearance;
	Appearance.Color = Color;
	Appearance.Diameter = Diameter;
	Appearance.Thickness = Thickness;
	Appearance.OutlineColor = OutlineColor;
	Appearance.OutlineThickness = OutlineThickness;
	MyRing->SetAppearance(Appearance);
}

void UHUDRing::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRing.Reset();
}

#if WITH_EDITOR
const FText UHUDRing::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "GDTSurvivor");
}
#endif

#undef LOCTEXT_NAMESPACE
