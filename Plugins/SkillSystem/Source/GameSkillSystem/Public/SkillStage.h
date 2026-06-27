// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SkillStage.generated.h"

class USkillTask;

/**
 * 阶段时间轴上的一条任务记录。
 * 瞬间任务请将 EndTime <= StartTime；
 * 持续任务（USustainSkillTask）需同时配置 Start/End 时间。
 */
USTRUCT(BlueprintType)
struct FSkillTimelineTask
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Instanced, Category = "Skill")
	TObjectPtr<USkillTask> Task = nullptr;

	UPROPERTY(EditAnywhere, Category = "Skill")
	float StartTime = 0.f;

	/** 仅用于持续任务。当此値 <= StartTime 时忽略。 */
	UPROPERTY(EditAnywhere, Category = "Skill")
	float EndTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "Skill")
	FName Id;
};

USTRUCT(BlueprintType)
struct FSkillEventBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Skill")
	FName EventName;

	UPROPERTY(EditAnywhere, Instanced, Category = "Skill")
	TObjectPtr<USkillTask> Task = nullptr;
};

/**
 * 定义技能的一个阶段：其时间域、时间轴任务、事件映射表和阶段切换规则。
 * 阶段本身不推进时间；时间由 Runner 推进。
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class GAMESKILLSYSTEM_API USkillStage : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	FName StageName;

	/** 技能编辑器图 / 时间轴中使用的用户可见名称。 */
	UPROPERTY(EditAnywhere, Category = "Skill Stage|Editor")
	FText DisplayName;

	/** 若 DisplayName 已设置则返回，否则依次回退到 StageName / 对象名称。 */
	FText GetDisplayNameText() const
	{
		if (!DisplayName.IsEmpty()) return DisplayName;
		if (!StageName.IsNone()) return FText::FromName(StageName);
		return FText::FromString(GetName());
	}

	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	float Duration = 1.f;

	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	TArray<FSkillTimelineTask> Timeline;

	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	TArray<FSkillEventBinding> EventMap;

	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	int32 DefaultNextStageIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	bool bAllowInterrupt = true;

	UPROPERTY(EditAnywhere, Category = "Skill Stage")
	bool bAllowStageChange = true;

	/** 本阶段内任务可选的共享运行时数据块大小。 */
	UPROPERTY(EditAnywhere, Category = "Skill Stage|Data")
	int32 StageDataSize = 0;

	UPROPERTY(EditAnywhere, Category = "Skill Stage|Data")
	int32 StageDataAlignment = 1;

	/** 查找指定事件名对应的任务，没有绑定时返回 null。 */
	USkillTask* FindEventTask(FName EventName) const;
};
