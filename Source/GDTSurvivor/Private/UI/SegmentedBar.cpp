#include "UI/SegmentedBar.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "SegmentedBar"

namespace
{
	// Copy of the USegmentedBar properties, so the Slate widget does not read the UObject while painting.
	struct FSegmentedBarAppearance
	{
		int32 SegmentCount = 20;
		ESegmentShape Shape = ESegmentShape::Block;
		float SegmentGap = 3.f;
		float EmptyOpacity = 0.15f;
		FLinearColor FrameColor = FLinearColor::White;
		float FrameThickness = 1.f;
		FMargin FramePadding;
		float CornerCut = 6.f;
		float BarHeight = 18.f;
		FText Label;
		FSlateFontInfo LabelFont;
		FLinearColor LabelColor = FLinearColor::White;
		float LabelSpacing = 4.f;
	};

	// Points of a half circle (or any arc) around Center, appended to Points. Angles in degrees, 0 = right, 90 = down.
	void AddArc(TArray<FVector2f>& Points, const FVector2f& Center, float Radius, float StartDegrees, float EndDegrees)
	{
		constexpr int32 Steps = 12;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Radians = FMath::DegreesToRadians(FMath::Lerp(StartDegrees, EndDegrees, static_cast<float>(Step) / Steps));
			Points.Add(Center + Radius * FVector2f(FMath::Cos(Radians), FMath::Sin(Radians)));
		}
	}
}

class SSegmentedBar : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSegmentedBar)
		: _Percent(0.f)
		, _FillColor(FLinearColor::White)
	{}
		SLATE_ATTRIBUTE(float, Percent)
		SLATE_ATTRIBUTE(FLinearColor, FillColor)
	SLATE_END_ARGS()

	SSegmentedBar()
		: PillBrush(FLinearColor::White, 0.f)
		, BlockBrush(FLinearColor::White)
	{
		PillBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	}

	void Construct(const FArguments& InArgs)
	{
		Percent = InArgs._Percent;
		FillColor = InArgs._FillColor;
	}

	void SetAppearance(const FSegmentedBarAppearance& InAppearance)
	{
		Appearance = InAppearance;
		Invalidate(EInvalidateWidgetReason::Layout);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float MinSegmentWidth = 4.f;
		const float Width = Appearance.SegmentCount * (MinSegmentWidth + Appearance.SegmentGap) + Appearance.FramePadding.GetTotalSpaceAlong<Orient_Horizontal>();
		return FVector2D(Width, MeasureLabel().Y * 0.5f + Appearance.BarHeight);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		const FVector2f Size = AllottedGeometry.GetLocalSize();
		const FVector2f LabelSize = MeasureLabel();

		// The top frame line runs through the middle of the label.
		const float FrameTop = LabelSize.Y * 0.5f;

		PaintFrame(AllottedGeometry, OutDrawElements, LayerId, DrawEffects, InWidgetStyle, Size, FrameTop, LabelSize);
		PaintSegments(AllottedGeometry, OutDrawElements, LayerId, DrawEffects, InWidgetStyle, Size, FrameTop, LabelSize);

		return LayerId + 1;
	}

private:
	FSlateFontInfo GetLabelFont() const
	{
		return Appearance.LabelFont.HasValidFont() ? Appearance.LabelFont : FCoreStyle::GetDefaultFontStyle("Regular", 9);
	}

	FVector2f MeasureLabel() const
	{
		if (Appearance.Label.IsEmpty() || !FSlateApplication::IsInitialized())
		{
			return FVector2f::ZeroVector;
		}
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FVector2f(FontMeasure->Measure(Appearance.Label, GetLabelFont()));
	}

	void PaintFrame(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, ESlateDrawEffect DrawEffects,
		const FWidgetStyle& InWidgetStyle, const FVector2f& Size, float FrameTop, const FVector2f& LabelSize) const
	{
		const float Thickness = Appearance.FrameThickness;
		const float HalfThickness = Thickness * 0.5f;
		const float Left = HalfThickness;
		const float Right = Size.X - HalfThickness;
		const float Top = FrameTop + HalfThickness;
		const float Bottom = Size.Y - HalfThickness;
		const bool bPill = Appearance.Shape == ESegmentShape::Pill;
		const float Radius = (Bottom - Top) * 0.5f;
		const float Cut = bPill ? 0.f : FMath::Min(Appearance.CornerCut, Radius);

		// The label sits in a gap of the top line, just after the corner.
		const float LabelGap = LabelSize.X > 0.f ? 4.f : 0.f;
		const float LabelStart = (bPill ? Left + Radius : Left + Cut) + 6.f;
		const float LabelEnd = LabelStart + LabelSize.X + 2.f * LabelGap;

		// One open line, clockwise from the right end of the label gap to its left end.
		TArray<FVector2f> Points;
		Points.Add(FVector2f(LabelSize.X > 0.f ? LabelEnd : LabelStart, Top));
		if (bPill)
		{
			Points.Add(FVector2f(Right - Radius, Top));
			AddArc(Points, FVector2f(Right - Radius, Top + Radius), Radius, -90.f, 90.f);
			Points.Add(FVector2f(Left + Radius, Bottom));
			AddArc(Points, FVector2f(Left + Radius, Top + Radius), Radius, 90.f, 270.f);
		}
		else
		{
			Points.Add(FVector2f(Right, Top));
			Points.Add(FVector2f(Right, Bottom - Cut));
			Points.Add(FVector2f(Right - Cut, Bottom));
			Points.Add(FVector2f(Left, Bottom));
			Points.Add(FVector2f(Left, Top + Cut));
			Points.Add(FVector2f(Left + Cut, Top));
		}
		Points.Add(FVector2f(LabelStart, Top));

		const FLinearColor FrameColor = Appearance.FrameColor * InWidgetStyle.GetColorAndOpacityTint();
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Points, DrawEffects, FrameColor, true, Thickness);

		if (LabelSize.X > 0.f)
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(LabelSize, FSlateLayoutTransform(FVector2f(LabelStart + LabelGap, 0.f))),
				Appearance.Label, GetLabelFont(), DrawEffects, Appearance.LabelColor * InWidgetStyle.GetColorAndOpacityTint());
		}
	}

	void PaintSegments(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, ESlateDrawEffect DrawEffects,
		const FWidgetStyle& InWidgetStyle, const FVector2f& Size, float FrameTop, const FVector2f& LabelSize) const
	{
		const FMargin& Padding = Appearance.FramePadding;
		const int32 Count = FMath::Max(Appearance.SegmentCount, 1);

		// The lower half of the label hangs into the frame; keep the segments below it.
		const float TopPadding = LabelSize.Y > 0.f
			? FMath::Max(Padding.Top, LabelSize.Y * 0.5f + Appearance.LabelSpacing)
			: Padding.Top;
		const FVector2f InnerPosition(Padding.Left, FrameTop + TopPadding);
		const FVector2f InnerSize(Size.X - Padding.GetTotalSpaceAlong<Orient_Horizontal>(), Size.Y - FrameTop - TopPadding - Padding.Bottom);
		const float SegmentWidth = (InnerSize.X - Appearance.SegmentGap * (Count - 1)) / Count;
		if (SegmentWidth <= 0.f || InnerSize.Y <= 0.f)
		{
			return;
		}

		const FSlateBrush* Brush = Appearance.Shape == ESegmentShape::Pill ? static_cast<const FSlateBrush*>(&PillBrush) : &BlockBrush;
		const FLinearColor Color = FillColor.Get() * InWidgetStyle.GetColorAndOpacityTint();
		const float FilledSegments = FMath::Clamp(Percent.Get(), 0.f, 1.f) * Count;

		for (int32 Index = 0; Index < Count; ++Index)
		{
			// 1 = full, 0 = empty, in between for the segment that holds the current value.
			const float Fill = FMath::Clamp(FilledSegments - Index, 0.f, 1.f);
			FLinearColor SegmentColor = Color;
			SegmentColor.A *= FMath::Lerp(Appearance.EmptyOpacity, 1.f, Fill);

			const FVector2f Position = InnerPosition + FVector2f(Index * (SegmentWidth + Appearance.SegmentGap), 0.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(FVector2f(SegmentWidth, InnerSize.Y), FSlateLayoutTransform(Position)),
				Brush, DrawEffects, SegmentColor);
		}
	}

	TAttribute<float> Percent;
	TAttribute<FLinearColor> FillColor;
	FSegmentedBarAppearance Appearance;
	FSlateRoundedBoxBrush PillBrush;
	FSlateColorBrush BlockBrush;
};

TSharedRef<SWidget> USegmentedBar::RebuildWidget()
{
	MySegmentedBar = SNew(SSegmentedBar)
		.Percent_UObject(this, &USegmentedBar::GetPercent)
		.FillColor_UObject(this, &USegmentedBar::GetFillColorAndOpacity);
	return MySegmentedBar.ToSharedRef();
}

void USegmentedBar::SetLabel(const FText& InLabel)
{
	if (Label.EqualTo(InLabel))
	{
		return;
	}
	Label = InLabel;
	SynchronizeProperties();
}

void USegmentedBar::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!MySegmentedBar.IsValid())
	{
		return;
	}

	FSegmentedBarAppearance Appearance;
	Appearance.SegmentCount = SegmentCount;
	Appearance.Shape = SegmentShape;
	Appearance.SegmentGap = SegmentGap;
	Appearance.EmptyOpacity = EmptyOpacity;
	Appearance.FrameColor = FrameColor;
	Appearance.FrameThickness = FrameThickness;
	Appearance.FramePadding = FramePadding;
	Appearance.CornerCut = CornerCut;
	Appearance.BarHeight = BarHeight;
	Appearance.Label = Label;
	Appearance.LabelFont = LabelFont;
	Appearance.LabelColor = LabelColor;
	Appearance.LabelSpacing = LabelSpacing;
	MySegmentedBar->SetAppearance(Appearance);
}

void USegmentedBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySegmentedBar.Reset();
}

#if WITH_EDITOR
const FText USegmentedBar::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "GDTSurvivor");
}
#endif

#undef LOCTEXT_NAMESPACE
