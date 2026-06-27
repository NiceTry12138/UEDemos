// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "SkillSystemTypes.h"

class USkillComponent;
class USkillAsset;
class UWorld;
class AActor;
class UObject;
struct FSkillRunner;

/**
 * 传递给 USkillTask 回调的上下文结构。
 * 提供对当前技能 Runner 的访问，
 * 以及 Runner 连续内存缓冲中的每技能 / 每阶段 / 每任务运行时数据块。
 *
 * bCreateInstance == false 的任务必须将所有可变状态
 * 存入这些数据块，而非任务对象本身。
 */
struct GAMESKILLSYSTEM_API FSkillTaskContext
{
	USkillComponent* Component = nullptr;
	FSkillRunner* Runner = nullptr;
	USkillAsset* Skill = nullptr;

	int32 StageIndex = INDEX_NONE;
	/** 当前任务在活跍阶段编译后时间轴数组中的索引。 */
	int32 TimelineIndex = INDEX_NONE;

	float CurrentSkillTime = 0.f;
	float CurrentStageTime = 0.f;
	float DeltaTime = 0.f;

	/** 激活点传入的外部上下文缓存（World、Actor 等）。 */
	UWorld* World = nullptr;
	AActor* OwnerActor = nullptr;
	AActor* AvatarActor = nullptr;
	UObject* SourceObject = nullptr;

	/** 返回技能级运行时数据块的原始指针（大小为 0 时可能为 null）。 */
	void* GetSkillDataRaw() const;
	/** 返回当前阶段级运行时数据块的原始指针。 */
	void* GetStageDataRaw() const;
	/** 返回当前任务运行时数据块的原始指针。 */
	void* GetTaskDataRaw() const;

	template <typename T>
	T* GetSkillData() const { return static_cast<T*>(GetSkillDataRaw()); }

	template <typename T>
	T* GetStageData() const { return static_cast<T*>(GetStageDataRaw()); }

	template <typename T>
	T* GetTaskData() const { return static_cast<T*>(GetTaskDataRaw()); }

	/** 使用指定时机将事件入队到 Runner 的事件队列。 */
	void SendEvent(FName EventName, ESkillEventTiming Timing = ESkillEventTiming::Immediate) const;

	/** 请求阶段切换，实际切换在帧末处理。 */
	void RequestStageChange(int32 TargetStageIndex, EStageChangeReason Reason = EStageChangeReason::TaskRequested, int32 Priority = 0) const;

	/** 请求技能在本帧结束时完成。 */
	void RequestEndSkill() const;
};
