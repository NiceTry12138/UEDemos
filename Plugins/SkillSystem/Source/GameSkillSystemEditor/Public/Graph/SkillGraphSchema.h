// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "SkillGraphSchema.generated.h"

UCLASS()
class USkillGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	static const FName PC_Exec;

	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override {}
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;
};
