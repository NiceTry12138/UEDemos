// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SkillTask_BlueprintBase.h"
#include "SkillSystemTypes.h"
#include "SkillTaskBlueprintLibrary.generated.h"

class USkillAsset;
class USkillComponent;
class AActor;

/**
 * 蓝图可访问的技能任务执行上下文读取辅助函数库。
 *
 * 在 USkillTask_BlueprintBase 或 USustainSkillTask_BlueprintBase 蓝图的
 * BP_Execute / BP_OnStart / BP_OnTick / BP_OnEnd 中调用这些函数，
 * 将‘self’（任务节点）作为 Task 参数传入。
 *
 * 所有函数均为 pure（无执行引脚），在有效执行上下文之外
 * 调用时返回默认备用值。
 */
UCLASS()
class GAMESKILLSYSTEM_API USkillTaskBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	// -----------------------------------------------------------------------
	// 内部辅助函数 – 从任务对象中提取缓存的 FSkillTaskContext*。
	// 在有效执行帧之外调用时返回 nullptr。
	// -----------------------------------------------------------------------
	static FSkillTaskContext* GetContext(const USkillTask* Task);

public:
	// -----------------------------------------------------------------------
	// Actor 与 World
	// -----------------------------------------------------------------------

	/** 返回技能所在的 World。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static UWorld* GetSkillWorld(const USkillTask* Task);

	/** 返回持有 USkillComponent 激活此技能的 Actor。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static AActor* GetOwnerActor(const USkillTask* Task);

	/**
	 * 返回化身 Actor（例如控制器在 Pawn 上激活技能时，
	 * 化身 Actor 可与持有者 Actor 不同）。
	 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static AActor* GetAvatarActor(const USkillTask* Task);

	/** 返回激活上下文中存储的可选源对象。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static UObject* GetSourceObject(const USkillTask* Task);

	// -----------------------------------------------------------------------
	// 资产与组件
	// -----------------------------------------------------------------------

	/** 返回驱动该执行的 USkillAsset。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static USkillAsset* GetSkillAsset(const USkillTask* Task);

	/** 返回激活该技能的 USkillComponent。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static USkillComponent* GetSkillComponent(const USkillTask* Task);

	// -----------------------------------------------------------------------
	// 时间
	// -----------------------------------------------------------------------

	/** 技能开始后经过的时间（秒）。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static float GetCurrentSkillTime(const USkillTask* Task);

	/** 当前阶段开始后经过的时间（秒）。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static float GetCurrentStageTime(const USkillTask* Task);

	/** 当前帧的 DeltaTime。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static float GetDeltaTime(const USkillTask* Task);

	// -----------------------------------------------------------------------
	// 阶段信息
	// -----------------------------------------------------------------------

	/** 技能 Stages 数组中当前活跍阶段的索引。 */
	UFUNCTION(BlueprintPure, Category = "Skill Task|Context", meta = (DefaultToSelf = "Task"))
	static int32 GetStageIndex(const USkillTask* Task);

	// -----------------------------------------------------------------------
	// 行为
	// -----------------------------------------------------------------------

	/**
	 * 将命名事件入队到技能事件队列。
	 * @param Timing  事件相对当前帧的处理时机。
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Task|Actions", meta = (DefaultToSelf = "Task"))
	static void SendSkillEvent(const USkillTask* Task, FName EventName,
		ESkillEventTiming Timing = ESkillEventTiming::Immediate);

	/**
	 * 请求阶段切换，在当前帧结束时处理。
	 * @param TargetStageIndex  USkillAsset::Stages 数组中的目标索引。
	 * @param Priority          多个请求竞争时，优先级高的请求胜出。
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill Task|Actions", meta = (DefaultToSelf = "Task"))
	static void RequestStageChange(const USkillTask* Task, int32 TargetStageIndex,
		EStageChangeReason Reason = EStageChangeReason::TaskRequested, int32 Priority = 0);

	/** 请求当前帧结束时结束技能。 */
	UFUNCTION(BlueprintCallable, Category = "Skill Task|Actions", meta = (DefaultToSelf = "Task"))
	static void RequestEndSkill(const USkillTask* Task);
};
