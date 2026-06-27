// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "SkillGraph.generated.h"

class USkillAsset;

UCLASS()
class USkillGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	/** 拥有此图的技能资产。 */
	UPROPERTY()
	TWeakObjectPtr<USkillAsset> OwningAsset;

	/** 从资产阶段重建图的节点和线性引脚连接。 */
	void RebuildFromAsset();
};
