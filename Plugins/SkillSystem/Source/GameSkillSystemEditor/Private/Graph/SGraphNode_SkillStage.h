// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class USkillGraphNode_Stage;

/**
 * 技能阶段节点的 Slate 控件。
 * 标题显示阶段显示名，主体按 StartTime 排序列出所有任务的显示名。
 */
class SGraphNode_SkillStage : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_SkillStage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, USkillGraphNode_Stage* InNode);

	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;

protected:
	/** 从底层图节点获取节点标题文本，用于 Slate 属性绑定。 */
	FText GetNodeTitleText() const;

	TSharedRef<SWidget> BuildTaskList();
};
