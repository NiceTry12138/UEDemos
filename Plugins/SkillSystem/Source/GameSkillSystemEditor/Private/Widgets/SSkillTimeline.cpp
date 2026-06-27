// Copyright. GameSkillSystemEditor.
#include "Widgets/SSkillTimeline.h"
#include "SkillStage.h"
#include "SkillTask.h"

#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"

namespace SkillTimelineLayout
{
	constexpr float HandleHalfWidth = 4.f;
	constexpr float SquareHalfWidth = 8.f;
	constexpr float HeaderHeight = 18.f;
}

void SSkillTimeline::Construct(const FArguments& InArgs)
{
	OnTimingChanged = InArgs._OnTimingChanged;
	OnTaskSelectedDelegate = InArgs._OnTaskSelected;
}

void SSkillTimeline::SetStage(USkillStage* InStage)
{
	StageWeak = InStage;
	DraggingTaskIndex = INDEX_NONE;
	DragMode = EDragMode::None;
	SelectedTaskIndex = INDEX_NONE;
	Invalidate(EInvalidateWidget::Layout);
}

void SSkillTimeline::RefreshLayout()
{
	Invalidate(EInvalidateWidget::Layout);
}

float SSkillTimeline::TimeToX(const FGeometry& Geometry, float Time) const
{
	USkillStage* Stage = StageWeak.Get();
	if (!Stage || Stage->Duration <= 0.f) return GetTimelineLeft();
	const float L = GetTimelineLeft();
	const float R = GetTimelineRight(Geometry);
	const float Frac = FMath::Clamp(Time / Stage->Duration, 0.f, 1.f);
	return FMath::Lerp(L, R, Frac);
}

float SSkillTimeline::XToTime(const FGeometry& Geometry, float X) const
{
	USkillStage* Stage = StageWeak.Get();
	if (!Stage || Stage->Duration <= 0.f) return 0.f;
	const float L = GetTimelineLeft();
	const float R = GetTimelineRight(Geometry);
	const float Frac = FMath::Clamp((X - L) / FMath::Max(1.f, R - L), 0.f, 1.f);
	return Frac * Stage->Duration;
}

int32 SSkillTimeline::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	using namespace SkillTimelineLayout;

	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush("WhiteBrush");
	const FSlateFontInfo Font = FCoreStyle::Get().GetFontStyle("NormalFont");
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	USkillStage* Stage = StageWeak.Get();
	if (!Stage)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2D(8, 8), FVector2D(200, 20)),
			TEXT("(No stage selected)"), Font, ESlateDrawEffect::None, FLinearColor(0.7f, 0.7f, 0.7f));
		return LayerId;
	}

	// --- 背景 -------------------------------------------------------
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
		WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.08f, 0.08f, 0.08f, 1.f));

	// --- 头部（尺尺）--------------------------------------------------
	const float L = GetTimelineLeft();
	const float R = GetTimelineRight(AllottedGeometry);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(L, 0), FVector2D(R - L, HeaderHeight)),
		WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.15f, 0.15f, 0.15f));

	// 每 0.1 秒一个刻度标记。
	const float Duration = FMath::Max(0.001f, Stage->Duration);
	for (float t = 0.f; t <= Duration + KINDA_SMALL_NUMBER; t += 0.1f)
	{
		const float X = TimeToX(AllottedGeometry, t);
		const bool bMajor = FMath::IsNearlyEqual(FMath::Fmod(t, 0.5f), 0.f, 0.01f) || FMath::IsNearlyEqual(FMath::Fmod(t, 0.5f), 0.5f, 0.01f);
		const float TickH = bMajor ? 14.f : 6.f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(X, HeaderHeight - TickH), FVector2D(1, TickH)),
			WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.6f, 0.6f, 0.6f));
		if (bMajor)
		{
			FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2D(X + 2, 0), FVector2D(40, 14)),
				FString::Printf(TEXT("%.2f"), t), Font, ESlateDrawEffect::None, FLinearColor(0.7f, 0.7f, 0.7f));
		}
	}

	// --- 行 ---------------------------------------------------------------
	const float RowH = GetRowHeight();
	for (int32 RowIdx = 0; RowIdx < Stage->Timeline.Num(); ++RowIdx)
	{
		const int32 Index = RowIdx;
		const FSkillTimelineTask& Entry = Stage->Timeline[Index];
		const float Y = HeaderHeight + RowIdx * RowH;

		// 行背景。
		const bool bIsSelected = (Index == SelectedTaskIndex);
		const FLinearColor RowBg = bIsSelected
			? FLinearColor(0.15f, 0.28f, 0.50f)
			: ((RowIdx & 1) ? FLinearColor(0.12f, 0.12f, 0.12f) : FLinearColor(0.10f, 0.10f, 0.10f));
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(0, Y), FVector2D(AllottedGeometry.GetLocalSize().X, RowH - 2)),
			WhiteBrush, ESlateDrawEffect::None, RowBg);

		// 左侧：名称列。
		FText Name = Entry.Task ? Entry.Task->GetDisplayNameText() : FText::FromString(TEXT("<None>"));
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(8, Y + 4), FVector2D(L - 16, RowH - 4)),
			Name, Font, ESlateDrawEffect::None, FLinearColor::White);

		// 右侧：时间轴部分。
		const bool bSustain = Entry.Task && Entry.Task->IsSustainTask();
		const float StartX = TimeToX(AllottedGeometry, Entry.StartTime);
		if (bSustain)
		{
			const float EndT = (Entry.EndTime > Entry.StartTime) ? Entry.EndTime : Entry.StartTime;
			const float EndX = TimeToX(AllottedGeometry, EndT);
			const float BarY = Y + 6;
			const float BarH = RowH - 12;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2D(StartX, BarY), FVector2D(FMath::Max(2.f, EndX - StartX), BarH)),
				WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.2f, 0.55f, 0.95f, 0.9f));
			// 边缘拖动把手。
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2D(StartX - HandleHalfWidth, BarY), FVector2D(HandleHalfWidth * 2, BarH)),
				WhiteBrush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.8f));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2D(EndX - HandleHalfWidth, BarY), FVector2D(HandleHalfWidth * 2, BarH)),
				WhiteBrush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.8f));
		}
		else
		{
			const float SquareY = Y + (RowH - 16) * 0.5f;
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2D(StartX - SquareHalfWidth, SquareY), FVector2D(SquareHalfWidth * 2, 16)),
				WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.95f, 0.7f, 0.2f, 0.95f));
		}
	}

	return LayerId + 5;
}

FReply SSkillTimeline::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	using namespace SkillTimelineLayout;

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	bMouseMoved = false;
	USkillStage* Stage = StageWeak.Get();
	if (!Stage) return FReply::Unhandled();

	const FVector2D Local = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (Local.Y < HeaderHeight) return FReply::Unhandled();

	const int32 Row = FMath::FloorToInt((Local.Y - HeaderHeight) / GetRowHeight());

	if (!Stage->Timeline.IsValidIndex(Row)) return FReply::Unhandled();

	const int32 Index = Row;
	FSkillTimelineTask& Entry = Stage->Timeline[Index];
	const bool bSustain = Entry.Task && Entry.Task->IsSustainTask();

	const float StartX = TimeToX(Geometry, Entry.StartTime);
	const float EndX   = TimeToX(Geometry, FMath::Max(Entry.EndTime, Entry.StartTime));
	const float ClickX = Local.X;

	DraggingTaskIndex = Index;

	if (bSustain)
	{
		if (FMath::Abs(ClickX - StartX) <= HandleHalfWidth + 2.f) DragMode = EDragMode::MoveStart;
		else if (FMath::Abs(ClickX - EndX) <= HandleHalfWidth + 2.f) DragMode = EDragMode::MoveEnd;
		else if (ClickX > StartX && ClickX < EndX) { DragMode = EDragMode::MoveSustain; DragGrabTimeOffset = XToTime(Geometry, ClickX) - Entry.StartTime; }
		else { DraggingTaskIndex = INDEX_NONE; DragMode = EDragMode::None; return FReply::Unhandled(); }
	}
	else
	{
		if (FMath::Abs(ClickX - StartX) <= SquareHalfWidth + 2.f) DragMode = EDragMode::MoveInstant;
		else { DraggingTaskIndex = INDEX_NONE; DragMode = EDragMode::None; return FReply::Unhandled(); }
	}

	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SSkillTimeline::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if (DragMode == EDragMode::None || DraggingTaskIndex == INDEX_NONE) return FReply::Unhandled();

	bMouseMoved = true;

	USkillStage* Stage = StageWeak.Get();
	if (!Stage || !Stage->Timeline.IsValidIndex(DraggingTaskIndex)) return FReply::Unhandled();

	FSkillTimelineTask& Entry = Stage->Timeline[DraggingTaskIndex];
	const FVector2D Local = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const float Time = XToTime(Geometry, Local.X);

	switch (DragMode)
	{
	case EDragMode::MoveInstant:
		Entry.StartTime = FMath::Clamp(Time, 0.f, Stage->Duration);
		break;
	case EDragMode::MoveStart:
		Entry.StartTime = FMath::Clamp(Time, 0.f, Entry.EndTime);
		break;
	case EDragMode::MoveEnd:
		Entry.EndTime = FMath::Clamp(Time, Entry.StartTime, Stage->Duration);
		break;
	case EDragMode::MoveSustain:
	{
		const float Width = Entry.EndTime - Entry.StartTime;
		float NewStart = FMath::Clamp(Time - DragGrabTimeOffset, 0.f, Stage->Duration - Width);
		Entry.StartTime = NewStart;
		Entry.EndTime = NewStart + Width;
		break;
	}
	default: break;
	}

	OnTimingChanged.ExecuteIfBound();
	Invalidate(EInvalidateWidget::Paint);
	return FReply::Handled();
}

FReply SSkillTimeline::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const bool bWasDragging = (DragMode != EDragMode::None);
	const int32 ClickedIndex = DraggingTaskIndex;
	DragMode = EDragMode::None;
	DraggingTaskIndex = INDEX_NONE;

	if (bWasDragging)
	{
		if (!bMouseMoved)
		{
			// 没有移动的点击：选中该任务并通知属性面板。
			SelectedTaskIndex = ClickedIndex;
			USkillStage* Stage = StageWeak.Get();
			if (Stage && Stage->Timeline.IsValidIndex(SelectedTaskIndex))
			{
				OnTaskSelectedDelegate.ExecuteIfBound(Stage->Timeline[SelectedTaskIndex].Task.Get());
			}
			Invalidate(EInvalidateWidget::Paint);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}
