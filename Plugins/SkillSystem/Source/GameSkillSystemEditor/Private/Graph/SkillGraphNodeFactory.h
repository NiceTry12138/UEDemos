// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * 图面板节点工厂：为技能阶段节点提供自定义 Slate 控件，
 * 使节点主体能展示该阶段的任务列表。
 */
class FSkillGraphNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* InNode) const override;
};
