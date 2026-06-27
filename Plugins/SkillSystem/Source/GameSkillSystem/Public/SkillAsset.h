// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillRuntimeLayout.h"
#include "SkillStage.h"
#include "SkillAsset.generated.h"

class USkillStage;
class USkillTask;

/**
 * 技能的静态描述数据：包含阶段、技能级事件映射表以及编译后的运行时内存布局。
 * 不保存任何运行时状态。
 */
UCLASS(BlueprintType)
class GAMESKILLSYSTEM_API USkillAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Skill")
	FName SkillId;

	UPROPERTY(EditAnywhere, Instanced, Category = "Skill")
	TArray<TObjectPtr<USkillStage>> Stages;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TArray<FSkillEventBinding> SkillEventMap;

	/** 可选的技能级共享运行时数据块。 */
	UPROPERTY(EditAnywhere, Category = "Skill|Data")
	int32 SkillDataSize = 0;

	UPROPERTY(EditAnywhere, Category = "Skill|Data")
	int32 SkillDataAlignment = 1;

	/** 在技能编辑器预览视口中预览该技能时生成的 Actor 类。 */
	UPROPERTY(EditAnywhere, Category = "Skill|Preview")
	TSubclassOf<AActor> PreviewActorClass;

	/** 用户可见的显示名称。 */
	UPROPERTY(EditAnywhere, Category = "Skill|Editor")
	FText DisplayName;

	/** 编译后的运行时内存布局，通过 EnsureCompiled() 惰性填充。 */
	UPROPERTY(Transient)
	FSkillRuntimeLayout CompiledLayout;

	/** 从编辑时数据编译（或重新编译）运行时内存布局。 */
	void EnsureCompiled();
	void Compile();

	/** 在技能层级查找指定事件名对应的任务。 */
	USkillTask* FindSkillEventTask(FName EventName) const;

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
