// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "SkillTask.h"
#include "SkillSystemTypes.h"
#include "SkillTask_BlueprintBase.generated.h"

class USkillTaskBlueprintLibrary;

/**
 * USkillTask 的蓝图可继承包装（瞬间任务）。
 *
 * 在蓝图中重写 BP_Execute 实现任务逻辑。
 * C++ 子类应直接通过 USkillTask 重写 OnExecute。
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class GAMESKILLSYSTEM_API USkillTask_BlueprintBase : public USkillTask
{
	GENERATED_BODY()

public:
	/**
	 * 任务被触发时调用的蓝图事件。
	 * 在蓝图子类中重写以实现任务逻辑。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Task")
	void BP_Execute();

	// USkillTask interface
	virtual void OnExecute(FSkillTaskContext& Context) override;

private:
	friend class USkillTaskBlueprintLibrary;
	FSkillTaskContext* CachedContext = nullptr;
};

/**
 * USustainSkillTask 的蓝图可继承包装（Start/Tick/End 生命周期）。
 *
 * 在蓝图子类中重写 BP_OnStart / BP_OnTick / BP_OnEnd。
 * BP_Execute 映射到 OnExecute 路径（持续任务通常不使用，
 * 但为与 USkillTask_BlueprintBase 保持对称性保留）。
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class GAMESKILLSYSTEM_API USustainSkillTask_BlueprintBase : public USustainSkillTask
{
	GENERATED_BODY()

public:
	/** 持续任务激活时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Task")
	void BP_OnStart();

	/** 持续任务活跍期间每帧调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Task")
	void BP_OnTick(float DeltaTime);

	/** 持续任务结束时调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Task")
	void BP_OnEnd(ESkillTaskEndReason EndReason);

	/**
	 * 持续任务以瞬间事件方式被触发时执行
	 * （不常见，为与 USkillTask_BlueprintBase 保持对称性保留）。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Task")
	void BP_Execute();

	// USustainSkillTask interface
	virtual void OnExecute(FSkillTaskContext& Context) override;
	virtual void OnStart(FSkillTaskContext& Context) override;
	virtual void OnTick(FSkillTaskContext& Context, float DeltaTime) override;
	virtual void OnEnd(FSkillTaskContext& Context, ESkillTaskEndReason EndReason) override;

private:
	friend class USkillTaskBlueprintLibrary;
	FSkillTaskContext* CachedContext = nullptr;
};
