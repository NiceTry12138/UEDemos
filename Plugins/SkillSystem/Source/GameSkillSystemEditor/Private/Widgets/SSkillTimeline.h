// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class USkillStage;
class USkillTask;

/**
 * 单个 SkillStage 的简洁时间轴控件。
 * 每个时间轴任务呈现为一行：左侧显示任务显示名，右侧为可拖动展示：
 *   - USkillTask         -> 在 StartTime 处显示可拖动的方块
 *   - USustainSkillTask  -> StartTime 到 EndTime 之间的可伸缩条
 *
 * 拖动会修改阶段中底层 FSkillTimelineTask 的时间数据，
 * 并通过 OnTimingChanged 通知工具套件将资产标记为腄。
 */
class SSkillTimeline : public SCompoundWidget
{
public:
	DECLARE_DELEGATE(FOnTimingChanged);
	/** 用户点击时间轴行且没有拖动时触发。 */
	DECLARE_DELEGATE_OneParam(FOnTaskSelected, USkillTask*);

	SLATE_BEGIN_ARGS(SSkillTimeline) {}
	SLATE_EVENT(FOnTimingChanged, OnTimingChanged)
	SLATE_EVENT(FOnTaskSelected, OnTaskSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetStage(USkillStage* InStage);
	void RefreshLayout();

	// SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(600, 200); }

private:
	enum class EDragMode : uint8 { None, MoveInstant, MoveStart, MoveEnd, MoveSustain };

	TWeakObjectPtr<USkillStage> StageWeak;
	FOnTimingChanged OnTimingChanged;

	// 布局辅助函数 ----------------------------------------------------
	float GetTimelineLeft() const { return 160.f; } // 左侧‘名称’列所占的像素宽
	float GetTimelineRight(const FGeometry& Geometry) const { return Geometry.GetLocalSize().X - 16.f; }
	float TimeToX(const FGeometry& Geometry, float Time) const;
	float XToTime(const FGeometry& Geometry, float X) const;
	float GetRowHeight() const { return 26.f; }

	// 拖动状态 ----------------------------------------------------------
	int32 DraggingTaskIndex = INDEX_NONE;
	EDragMode DragMode = EDragMode::None;
	float DragGrabTimeOffset = 0.f;
	bool bMouseMoved = false;

	// 选中状态 ----------------------------------------------------------
	int32 SelectedTaskIndex = INDEX_NONE;
	FOnTaskSelected OnTaskSelectedDelegate;
};
