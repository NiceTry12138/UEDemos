// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "SkillGraphNodes.generated.h"

class USkillStage;

/** 技能编辑器图节点的公共基类。 */
UCLASS(Abstract)
class USkillGraphNode_Base : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UEdGraphPin* GetExecInputPin() const;
	UEdGraphPin* GetExecOutputPin() const;

	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
};

UCLASS()
class USkillGraphNode_Start : public USkillGraphNode_Base
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor(0.1f, 0.5f, 0.1f); }
};

UCLASS()
class USkillGraphNode_End : public USkillGraphNode_Base
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor(0.5f, 0.1f, 0.1f); }
};

UCLASS()
class USkillGraphNode_Stage : public USkillGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TWeakObjectPtr<USkillStage> Stage;

	UPROPERTY()
	int32 StageIndex = INDEX_NONE;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor(0.1f, 0.3f, 0.6f); }
};
