// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "SkillSystemTypes.generated.h"

UENUM(BlueprintType)
enum class ESkillTaskEndReason : uint8
{
	NaturalEnd,
	StageChanged,
	SkillInterrupted,
	SkillFinished,
	OwnerDestroyed,
};

UENUM(BlueprintType)
enum class EStageChangeReason : uint8
{
	Default,
	TaskRequested,
	EventDriven,
	External,
};

UENUM(BlueprintType)
enum class ESkillEventTiming : uint8
{
	Immediate,
	FrameEnd,
	NextFrameBegin,
};

USTRUCT(BlueprintType)
struct FStageChangeRequest
{
	GENERATED_BODY()

	UPROPERTY()
	int32 TargetStageIndex = INDEX_NONE;

	UPROPERTY()
	int32 Priority = 0;

	UPROPERTY()
	EStageChangeReason Reason = EStageChangeReason::Default;
};

USTRUCT()
struct FSkillPendingEvent
{
	GENERATED_BODY()

	UPROPERTY()
	FName EventName;

	UPROPERTY()
	ESkillEventTiming Timing = ESkillEventTiming::Immediate;
};

/**
 * 激活时传递给 SkillRunner 的外部上下文。
 * 为任务提供 UWorld、持有者 Actor、化身 Actor
 * 以及任意用户定义的来源对象，以便任务
 * 展示 Debug、生成 Actor、查询物理等操作。
 */
USTRUCT(BlueprintType)
struct FSkillContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UWorld> World = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> OwnerActor = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> AvatarActor = nullptr;

	UPROPERTY()
	TObjectPtr<UObject> SourceObject = nullptr;
};
