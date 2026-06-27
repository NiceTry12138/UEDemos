// Copyright. GameSkillSystemEditor.
#include "Graph/SkillGraphSchema.h"
#include "Graph/SkillGraphNodes.h"

const FName USkillGraphSchema::PC_Exec(TEXT("Exec"));

FLinearColor USkillGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

const FPinConnectionResponse USkillGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pin"));
	if (A->GetOwningNode() == B->GetOwningNode()) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Same node"));
	if (A->Direction == B->Direction) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Direction"));
	// 只读图：禁止编辑连接（图从资产阶段列表自动生成）。
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Skill graph is auto-generated"));
}

bool USkillGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	return false;
}
