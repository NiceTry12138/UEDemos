// Copyright. GameSkillSystemEditor.
#include "Graph/SkillGraph.h"
#include "Graph/SkillGraphNodes.h"
#include "Graph/SkillGraphSchema.h"
#include "SkillAsset.h"
#include "SkillStage.h"

void USkillGraph::RebuildFromAsset()
{
	// 清除现有节点。
	TArray<UEdGraphNode*> Existing = Nodes;
	for (UEdGraphNode* N : Existing)
	{
		if (N) RemoveNode(N, /*bBreakAllLinks*/ true);
	}
	Nodes.Reset();

	if (!OwningAsset.IsValid()) return;
	USkillAsset* Asset = OwningAsset.Get();

	// 生成起始节点。
	USkillGraphNode_Start* Start = NewObject<USkillGraphNode_Start>(this);
	Start->CreateNewGuid();
	Start->NodePosX = 0;
	Start->NodePosY = 0;
	AddNode(Start, false, false);
	Start->AllocateDefaultPins();

	int32 X = 220;
	const int32 StepX = 240;

	UEdGraphPin* PrevOut = Start->GetExecOutputPin();
	for (int32 i = 0; i < Asset->Stages.Num(); ++i)
	{
		USkillGraphNode_Stage* Node = NewObject<USkillGraphNode_Stage>(this);
		Node->CreateNewGuid();
		Node->StageIndex = i;
		Node->Stage = Asset->Stages[i];
		Node->NodePosX = X;
		Node->NodePosY = 0;
		AddNode(Node, false, false);
		Node->AllocateDefaultPins();

		if (PrevOut)
		{
			if (UEdGraphPin* In = Node->GetExecInputPin())
			{
				PrevOut->MakeLinkTo(In);
			}
		}
		PrevOut = Node->GetExecOutputPin();
		X += StepX;
	}

	USkillGraphNode_End* End = NewObject<USkillGraphNode_End>(this);
	End->CreateNewGuid();
	End->NodePosX = X;
	End->NodePosY = 0;
	AddNode(End, false, false);
	End->AllocateDefaultPins();
	if (PrevOut)
	{
		if (UEdGraphPin* In = End->GetExecInputPin())
		{
			PrevOut->MakeLinkTo(In);
		}
	}

	NotifyGraphChanged();
}
