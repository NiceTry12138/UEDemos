// Copyright. GameSkillSystem.
#include "SkillStage.h"
#include "SkillTask.h"

USkillTask* USkillStage::FindEventTask(FName EventName) const
{
	for (const FSkillEventBinding& Binding : EventMap)
	{
		if (Binding.EventName == EventName)
		{
			return Binding.Task;
		}
	}
	return nullptr;
}
