// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "SkillRuntimeLayout.generated.h"

USTRUCT()
struct FSkillTaskDataLayout
{
	GENERATED_BODY()

	UPROPERTY() int32 TimelineIndex = INDEX_NONE;
	UPROPERTY() int32 DataOffset = 0;
	UPROPERTY() int32 DataSize = 0;
	UPROPERTY() int32 Alignment = 1;
	UPROPERTY() FName DebugName;
};

USTRUCT()
struct FSkillStageDataLayout
{
	GENERATED_BODY()

	UPROPERTY() int32 StageIndex = INDEX_NONE;
	UPROPERTY() int32 StageDataOffset = 0;
	UPROPERTY() int32 StageDataSize = 0;
	UPROPERTY() int32 StageDataAlignment = 1;

	/** 本阶段内每个任务的布局，索引与该阶段编译后的时间轴数组一一对应。 */
	UPROPERTY() TArray<FSkillTaskDataLayout> TaskLayouts;
};

USTRUCT()
struct FSkillRuntimeLayout
{
	GENERATED_BODY()

	UPROPERTY() int32 TotalSize = 0;
	UPROPERTY() int32 SkillDataOffset = 0;
	UPROPERTY() int32 SkillDataSize = 0;
	UPROPERTY() int32 SkillDataAlignment = 1;

	UPROPERTY() TArray<FSkillStageDataLayout> StageLayouts;

	UPROPERTY() bool bCompiled = false;
};
