// Copyright. GameSkillSystem.
#include "SkillAsset.h"
#include "SkillStage.h"
#include "SkillTask.h"

namespace SkillAssetCompile
{
	static int32 AlignUp(int32 Offset, int32 Alignment)
	{
		const int32 Safe = FMath::Max(1, Alignment);
		return ((Offset + Safe - 1) / Safe) * Safe;
	}
}

void USkillAsset::EnsureCompiled()
{
	if (!CompiledLayout.bCompiled)
	{
		Compile();
	}
}

void USkillAsset::Compile()
{
	using namespace SkillAssetCompile;

	CompiledLayout = FSkillRuntimeLayout();

	int32 Offset = 0;

		// 技能级数据块。
	if (SkillDataSize > 0)
	{
		Offset = AlignUp(Offset, SkillDataAlignment);
		CompiledLayout.SkillDataOffset = Offset;
		CompiledLayout.SkillDataSize = SkillDataSize;
		CompiledLayout.SkillDataAlignment = FMath::Max(1, SkillDataAlignment);
		Offset += SkillDataSize;
	}

	CompiledLayout.StageLayouts.Reserve(Stages.Num());
	for (int32 StageIdx = 0; StageIdx < Stages.Num(); ++StageIdx)
	{
		USkillStage* Stage = Stages[StageIdx];
		FSkillStageDataLayout StageLayout;
		StageLayout.StageIndex = StageIdx;

		if (Stage)
		{
			if (Stage->StageDataSize > 0)
			{
				Offset = AlignUp(Offset, Stage->StageDataAlignment);
				StageLayout.StageDataOffset = Offset;
				StageLayout.StageDataSize = Stage->StageDataSize;
				StageLayout.StageDataAlignment = FMath::Max(1, Stage->StageDataAlignment);
				Offset += Stage->StageDataSize;
			}

			StageLayout.TaskLayouts.Reserve(Stage->Timeline.Num());
			for (int32 TaskIdx = 0; TaskIdx < Stage->Timeline.Num(); ++TaskIdx)
			{
				const FSkillTimelineTask& Entry = Stage->Timeline[TaskIdx];
				FSkillTaskDataLayout TaskLayout;
				TaskLayout.TimelineIndex = TaskIdx;

				if (Entry.Task)
				{
					const int32 Size = Entry.Task->GetTaskDataSize();
					if (Size > 0)
					{
						const int32 Alignment = FMath::Max(1, Entry.Task->GetTaskDataAlignment());
						Offset = AlignUp(Offset, Alignment);
						TaskLayout.DataOffset = Offset;
						TaskLayout.DataSize = Size;
						TaskLayout.Alignment = Alignment;
						TaskLayout.DebugName = Entry.Task->DebugName.IsNone() ? Entry.Id : Entry.Task->DebugName;
						Offset += Size;
					}
				}

				StageLayout.TaskLayouts.Add(MoveTemp(TaskLayout));
			}
		}

		CompiledLayout.StageLayouts.Add(MoveTemp(StageLayout));
	}

	CompiledLayout.TotalSize = Offset;
	CompiledLayout.bCompiled = true;
}

USkillTask* USkillAsset::FindSkillEventTask(FName EventName) const
{
	for (const FSkillEventBinding& Binding : SkillEventMap)
	{
		if (Binding.EventName == EventName)
		{
			return Binding.Task;
		}
	}
	return nullptr;
}

void USkillAsset::PostLoad()
{
	Super::PostLoad();
	Compile();
}

#if WITH_EDITOR
void USkillAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	Compile();
}
#endif
