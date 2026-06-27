// Copyright. GameSkillSystem.
#include "SkillTask_BlueprintBase.h"

// ---------------------------------------------------------------------------
// USkillTask_BlueprintBase
// ---------------------------------------------------------------------------

void USkillTask_BlueprintBase::OnExecute(FSkillTaskContext& Context)
{
	CachedContext = &Context;
	BP_Execute();
	CachedContext = nullptr;
}

// ---------------------------------------------------------------------------
// USustainSkillTask_BlueprintBase
// ---------------------------------------------------------------------------

void USustainSkillTask_BlueprintBase::OnExecute(FSkillTaskContext& Context)
{
	CachedContext = &Context;
	BP_Execute();
	CachedContext = nullptr;
}

void USustainSkillTask_BlueprintBase::OnStart(FSkillTaskContext& Context)
{
	CachedContext = &Context;
	BP_OnStart();
	CachedContext = nullptr;
}

void USustainSkillTask_BlueprintBase::OnTick(FSkillTaskContext& Context, float DeltaTime)
{
	CachedContext = &Context;
	BP_OnTick(DeltaTime);
	CachedContext = nullptr;
}

void USustainSkillTask_BlueprintBase::OnEnd(FSkillTaskContext& Context, ESkillTaskEndReason EndReason)
{
	CachedContext = &Context;
	BP_OnEnd(EndReason);
	CachedContext = nullptr;
}
