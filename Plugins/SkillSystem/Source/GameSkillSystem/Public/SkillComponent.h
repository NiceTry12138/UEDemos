// Copyright. GameSkillSystem.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillSystemTypes.h"
#include "SkillComponent.generated.h"

class USkillAsset;
struct FSkillRunner;

/**
 * 持有角色/实体技能运行时数据的组件。
 * 职责：
 *   - 管理当前活跍的 FSkillRunner 实例集合。
 *   - 通过 Context.Component 向任务提供外部上下文（持有者 / 化身 / 战斗上下文等数据）。
 *   - Tick 所有活跍的 Runner，并清理已结束的 Runner。
 */
UCLASS(ClassGroup = (Skill), meta = (BlueprintSpawnableComponent))
class GAMESKILLSYSTEM_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 激活指定技能。启动失败时返回 false。 */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool ActivateSkill(USkillAsset* SkillAsset);

	/** 使用显式外部上下文激活指定技能。 */
	bool ActivateSkillWithContext(USkillAsset* SkillAsset, const FSkillContext& InContext);

	/** 自动构建技能上下文时使用的 Avatar Actor（默认为持有者 Actor）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TObjectPtr<AActor> AvatarActorOverride = nullptr;

	/** 取消所有运行中的技能（结束原因： SkillInterrupted）。 */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void CancelAllSkills();

	/** 取消第一个激活了指定资产的 Runner。 */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void CancelSkill(USkillAsset* SkillAsset);

	/** 向所有活跍的 Runner 发送事件。 */
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SendEventToAll(FName EventName, ESkillEventTiming Timing = ESkillEventTiming::Immediate);

	int32 GetActiveRunnerCount() const { return Runners.Num(); }

private:
	/** 活跍的 Runner 列表。非 UObject，使用 TUniquePtr 以在数组增长时保持稳定的指针。 */
	TArray<TUniquePtr<FSkillRunner>> Runners;
};
