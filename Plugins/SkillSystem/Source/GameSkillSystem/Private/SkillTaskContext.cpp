// Copyright. GameSkillSystem.
#include "SkillTaskContext.h"
#include "SkillRunner.h"

void* FSkillTaskContext::GetSkillDataRaw() const
{
	return Runner ? Runner->GetSkillDataRaw() : nullptr;
}

void* FSkillTaskContext::GetStageDataRaw() const
{
	return Runner ? Runner->GetStageDataRaw(StageIndex) : nullptr;
}

void* FSkillTaskContext::GetTaskDataRaw() const
{
	return Runner ? Runner->GetTaskDataRaw(StageIndex, TimelineIndex) : nullptr;
}

void FSkillTaskContext::SendEvent(FName EventName, ESkillEventTiming Timing) const
{
	if (Runner)
	{
		Runner->SendEvent(EventName, Timing);
	}
}

void FSkillTaskContext::RequestStageChange(int32 TargetStageIndex, EStageChangeReason Reason, int32 Priority) const
{
	if (Runner)
	{
		Runner->RequestStageChange(TargetStageIndex, Reason, Priority);
	}
}

void FSkillTaskContext::RequestEndSkill() const
{
	if (Runner)
	{
		Runner->RequestEndSkill();
	}
}
