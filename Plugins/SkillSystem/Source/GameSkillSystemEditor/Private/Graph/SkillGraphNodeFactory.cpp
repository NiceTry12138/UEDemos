// Copyright. GameSkillSystemEditor.
#include "Graph/SkillGraphNodeFactory.h"
#include "Graph/SkillGraphNodes.h"
#include "Graph/SGraphNode_SkillStage.h"

TSharedPtr<SGraphNode> FSkillGraphNodeFactory::CreateNode(UEdGraphNode* InNode) const
{
	if (USkillGraphNode_Stage* Stage = Cast<USkillGraphNode_Stage>(InNode))
	{
		// 工具套件在图编辑器创建后重新绑定自身（见 FSkillAssetEditorToolkit::BindStageNodeWidgets）。
		return SNew(SGraphNode_SkillStage, Stage);
	}
	return nullptr;
}
