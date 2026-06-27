// Copyright. GameSkillSystem.
#include "SkillTaskBlueprintLibrary.h"
#include "SkillTaskContext.h"
#include "SkillComponent.h"
#include "SkillAsset.h"

// ---------------------------------------------------------------------------
// 内部辅助函数
// ---------------------------------------------------------------------------

FSkillTaskContext* USkillTaskBlueprintLibrary::GetContext(const USkillTask* Task)
{
	if (!Task) return nullptr;

	if (const USkillTask_BlueprintBase* A = Cast<USkillTask_BlueprintBase>(Task))
	{
		return A->CachedContext;
	}
	if (const USustainSkillTask_BlueprintBase* B = Cast<USustainSkillTask_BlueprintBase>(Task))
	{
		return B->CachedContext;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// Actor 与 World
// ---------------------------------------------------------------------------

UWorld* USkillTaskBlueprintLibrary::GetSkillWorld(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->World;
	return nullptr;
}

AActor* USkillTaskBlueprintLibrary::GetOwnerActor(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->OwnerActor;
	return nullptr;
}

AActor* USkillTaskBlueprintLibrary::GetAvatarActor(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->AvatarActor;
	return nullptr;
}

UObject* USkillTaskBlueprintLibrary::GetSourceObject(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->SourceObject;
	return nullptr;
}

// ---------------------------------------------------------------------------
// 资产与组件
// ---------------------------------------------------------------------------

USkillAsset* USkillTaskBlueprintLibrary::GetSkillAsset(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->Skill;
	return nullptr;
}

USkillComponent* USkillTaskBlueprintLibrary::GetSkillComponent(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->Component;
	return nullptr;
}

// ---------------------------------------------------------------------------
// 时间
// ---------------------------------------------------------------------------

float USkillTaskBlueprintLibrary::GetCurrentSkillTime(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->CurrentSkillTime;
	return 0.f;
}

float USkillTaskBlueprintLibrary::GetCurrentStageTime(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->CurrentStageTime;
	return 0.f;
}

float USkillTaskBlueprintLibrary::GetDeltaTime(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->DeltaTime;
	return 0.f;
}

// ---------------------------------------------------------------------------
// 阶段信息
// ---------------------------------------------------------------------------

int32 USkillTaskBlueprintLibrary::GetStageIndex(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task)) return Ctx->StageIndex;
	return INDEX_NONE;
}

// ---------------------------------------------------------------------------
// 行为
// ---------------------------------------------------------------------------

void USkillTaskBlueprintLibrary::SendSkillEvent(const USkillTask* Task, FName EventName, ESkillEventTiming Timing)
{
	if (FSkillTaskContext* Ctx = GetContext(Task))
	{
		Ctx->SendEvent(EventName, Timing);
	}
}

void USkillTaskBlueprintLibrary::RequestStageChange(const USkillTask* Task, int32 TargetStageIndex,
	EStageChangeReason Reason, int32 Priority)
{
	if (FSkillTaskContext* Ctx = GetContext(Task))
	{
		Ctx->RequestStageChange(TargetStageIndex, Reason, Priority);
	}
}

void USkillTaskBlueprintLibrary::RequestEndSkill(const USkillTask* Task)
{
	if (FSkillTaskContext* Ctx = GetContext(Task))
	{
		Ctx->RequestEndSkill();
	}
}
