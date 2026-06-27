// Copyright. GameSkillSystemEditor.
#include "Graph/SkillGraphNodes.h"
#include "Graph/SkillGraphSchema.h"
#include "SkillStage.h"

#define LOCTEXT_NAMESPACE "SkillGraphNodes"

UEdGraphPin* USkillGraphNode_Base::GetExecInputPin() const
{
	for (UEdGraphPin* Pin : Pins) { if (Pin && Pin->Direction == EGPD_Input) return Pin; }
	return nullptr;
}

UEdGraphPin* USkillGraphNode_Base::GetExecOutputPin() const
{
	for (UEdGraphPin* Pin : Pins) { if (Pin && Pin->Direction == EGPD_Output) return Pin; }
	return nullptr;
}

// --- 起始节点 ----------------------------------------------------------------
void USkillGraphNode_Start::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, USkillGraphSchema::PC_Exec, TEXT("Out"));
}

FText USkillGraphNode_Start::GetNodeTitle(ENodeTitleType::Type) const
{
	return LOCTEXT("StartTitle", "Start");
}

// --- 结束节点 ------------------------------------------------------------------
void USkillGraphNode_End::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, USkillGraphSchema::PC_Exec, TEXT("In"));
}

FText USkillGraphNode_End::GetNodeTitle(ENodeTitleType::Type) const
{
	return LOCTEXT("EndTitle", "End");
}

// --- 阶段节点 ----------------------------------------------------------------
void USkillGraphNode_Stage::AllocateDefaultPins()
{
	CreatePin(EGPD_Input,  USkillGraphSchema::PC_Exec, TEXT("In"));
	CreatePin(EGPD_Output, USkillGraphSchema::PC_Exec, TEXT("Out"));
}

FText USkillGraphNode_Stage::GetNodeTitle(ENodeTitleType::Type) const
{
	if (Stage.IsValid())
	{
		return Stage->GetDisplayNameText();
	}
	return FText::Format(LOCTEXT("StageTitleFmt", "Stage {0}"), FText::AsNumber(StageIndex));
}

#undef LOCTEXT_NAMESPACE
