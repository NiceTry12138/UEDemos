// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "SkillSystemTypes.h"
#include "SkillTaskContext.h"

class USkillAsset;
class USkillComponent;
class USkillTask;
class USkillStage;

/**
 * 单个激活技能的执行单元。拥有运行时内存块，
 * 并按设计文档驱动时间轴 / 事件 / 阶段切换的处理。
 *
 * 非 UObject。通过 bCreateInstance 实例化的任务使用每类池中的
 * TStrongObjectPtr 保持存活。
 */
struct GAMESKILLSYSTEM_API FSkillRunner
{
	FSkillRunner();
	~FSkillRunner();

	// --- 生命周期 -------------------------------------------------------

	bool Initialize(USkillComponent* InComponent, USkillAsset* InSkill);
	void Tick(float DeltaTime);
	void Finish(ESkillTaskEndReason Reason);

	bool IsFinished() const { return bFinished; }
	USkillAsset* GetSkill() const { return Skill; }

	void SetSkillContext(const FSkillContext& InContext) { SkillCtx = InContext; }
	const FSkillContext& GetSkillContext() const { return SkillCtx; }

	// --- 外部 API ----------------------------------------------------

	void SendEvent(FName EventName, ESkillEventTiming Timing);
	void RequestStageChange(int32 TargetStageIndex, EStageChangeReason Reason, int32 Priority);
	void RequestEndSkill();

	// --- 运行时内存访问器 ------------------------------------------

	void* GetSkillDataRaw() const;
	void* GetStageDataRaw(int32 StageIndex) const;
	void* GetTaskDataRaw(int32 StageIndex, int32 TimelineIndex) const;

private:
	struct FActiveSustain
	{
		int32 TimelineIndex = INDEX_NONE;
		bool bStarted = false;
		bool bEnded = false;
	};

	struct FPendingEvent
	{
		FName EventName;
	};

	// Tick 阶段 ---------------------------------------------------------
	void Tick_FrameBegin();
	void Tick_AdvanceTimeline(float DeltaTime);
	void Tick_DrainImmediateQueue();
	void Tick_FrameEnd();
	void Tick_ProcessStageChange();

	// 辅助函数 --------------------------------------------------------------
	USkillStage* GetCurrentStage() const;
	USkillStage* GetStage(int32 Index) const;

	void EnterStage(int32 NewStageIndex);
	void ExitCurrentStage(ESkillTaskEndReason Reason);

	void ExecuteEventTask(USkillTask* Task, FName EventName);
	void ExecuteInstantTimelineTask(int32 TimelineIndex);

	void StartSustain(int32 TimelineIndex);
	void TickSustain(int32 TimelineIndex, float DeltaTime);
	void EndSustain(int32 TimelineIndex, ESkillTaskEndReason Reason);

	void EndAllActiveSustainTasks(ESkillTaskEndReason Reason);

	/** 返回任务的 CDO 或池化实例，取决于 bCreateInstance。 */
	USkillTask* AcquireTask(USkillTask* Source, int32 StageIndex, int32 TimelineIndex);
	/** 将 AcquireTask 创建的实例归还至池。 */
	void ReleaseAllInstances();

	void BuildTaskContext(FSkillTaskContext& OutCtx, int32 StageIndex, int32 TimelineIndex, float DeltaTime) const;

	// 状态 ---------------------------------------------------------------
	USkillComponent* Component = nullptr;
	USkillAsset* Skill = nullptr;

	/** 激活时传入的外部上下文（World、持有者、化身 Actor 等）。 */
	FSkillContext SkillCtx;

	int32 CurrentStageIndex = INDEX_NONE;
	float CurrentSkillTime = 0.f;
	float CurrentStageTime = 0.f;
	bool bFinished = false;
	bool bRequestEnd = false;

	/** 连续的运行时内存：SkillData + StageData + TaskData 块。 */
	TArray<uint8> RuntimeMemory;

	TArray<FActiveSustain> ActiveSustainTasks;
	int32 NextInstantCursor = 0;

	TArray<FPendingEvent> ImmediateQueue;
	TArray<FPendingEvent> FrameEndQueue;
	TArray<FPendingEvent> NextFrameBeginQueue;

	TOptional<FStageChangeRequest> PendingStageChange;

	/** 池：按类缓存已 Reset 的任务实例。 */
	TMap<UClass*, TArray<TStrongObjectPtr<USkillTask>>> InstancePool;
	/** 当前阶段时间轴索引为键的活跍实例。 */
	TMap<int32, TStrongObjectPtr<USkillTask>> ActiveInstances;
};
