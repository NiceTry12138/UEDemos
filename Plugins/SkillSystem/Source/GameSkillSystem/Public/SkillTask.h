// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SkillTaskContext.h"
#include "SkillTask.generated.h"

/**
 * 瞬间技能任务的基类。默认情况下任务无状态，在其 CDO 上运行；
 * 若任务需要每次执行的成员状态，请将 bCreateInstance 设为 true。
 *
 * 运行时状态优先放入通过 GetTaskDataSize/Alignment
 * 返回的 FXxxTaskData 结构中，并通过 Context.GetTaskData<T>()访问。
 */
UCLASS()
class GAMESKILLSYSTEM_API USkillTask : public UObject
{
	GENERATED_BODY()

public:
	/** 为 true 时，任务从 Runner 的每类对象池中分配实例。 */
	UPROPERTY(EditAnywhere, Category = "Skill Task")
	bool bCreateInstance = false;

	/** 可选的调试名称，在布局 / 运行时日志中显示。 */
	UPROPERTY(EditAnywhere, Category = "Skill Task")
	FName DebugName;

	/** 技能编辑器中使用的用户可见名称（图节点行、时间轴行等）。 */
	UPROPERTY(EditAnywhere, Category = "Skill Task|Editor")
	FText DisplayName;

	/** 若 DisplayName 已设置则返回，否则依次回退到 DebugName / 类名。 */
	FText GetDisplayNameText() const
	{
		if (!DisplayName.IsEmpty()) return DisplayName;
		if (!DebugName.IsNone()) return FText::FromName(DebugName);
		return FText::FromString(GetClass() ? GetClass()->GetName() : TEXT("SkillTask"));
	}

	/** 返回该任务每次运行的数据块大小（字节），默认为 0（无数据）。 */
	virtual int32 GetTaskDataSize() const { return 0; }

	/** 返回任务数据块的内存对齐。 */
	virtual int32 GetTaskDataAlignment() const { return 1; }

	/** 重置暂存状态。实例化任务归还到对象池时调用。 */
	virtual void Reset() {}

	/** 任务被触发时执行一次（时间轴点或事件）。 */
	virtual void OnExecute(FSkillTaskContext& Context) {}

	/** 如果该任务类是持续任务（具有 Start/Tick/End 生命周期）则返回 true。 */
	virtual bool IsSustainTask() const { return false; }
};

/**
 * 具有显式 Start/Tick/End 生命周期的任务基类。
 * 用于打击判定框、锁定、持续运动等持续性行为。
 */
UCLASS()
class GAMESKILLSYSTEM_API USustainSkillTask : public USkillTask
{
	GENERATED_BODY()

public:
	virtual bool IsSustainTask() const override { return true; }

	virtual void OnStart(FSkillTaskContext& Context) {}
	virtual void OnTick(FSkillTaskContext& Context, float DeltaTime) {}
	virtual void OnEnd(FSkillTaskContext& Context, ESkillTaskEndReason EndReason) {}
};
