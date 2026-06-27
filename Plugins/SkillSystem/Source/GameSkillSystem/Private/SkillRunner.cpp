// Copyright. GameSkillSystem.
#include "SkillRunner.h"
#include "SkillAsset.h"
#include "SkillStage.h"
#include "SkillTask.h"
#include "SkillComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSkillRunner, Log, All);

FSkillRunner::FSkillRunner() = default;
FSkillRunner::~FSkillRunner() = default;

// =============================================================
// 生命周期
// =============================================================

bool FSkillRunner::Initialize(USkillComponent* InComponent, USkillAsset* InSkill)
{
	if (!InComponent || !InSkill || InSkill->Stages.Num() == 0)
	{
		return false;
	}

	Component = InComponent;
	Skill = InSkill;
	Skill->EnsureCompiled();

	const FSkillRuntimeLayout& Layout = Skill->CompiledLayout;
	RuntimeMemory.SetNumZeroed(FMath::Max(0, Layout.TotalSize));

	CurrentSkillTime = 0.f;
	CurrentStageTime = 0.f;
	bFinished = false;
	bRequestEnd = false;
	NextInstantCursor = 0;
	ActiveSustainTasks.Reset();
	ImmediateQueue.Reset();
	FrameEndQueue.Reset();
	NextFrameBeginQueue.Reset();
	PendingStageChange.Reset();

	EnterStage(0);
	return true;
}

void FSkillRunner::Tick(float DeltaTime)
{
	if (bFinished)
	{
		return;
	}

	Tick_FrameBegin();
	Tick_AdvanceTimeline(DeltaTime);
	Tick_DrainImmediateQueue();
	Tick_FrameEnd();
	Tick_ProcessStageChange();

	if (!bFinished && bRequestEnd)
	{
		Finish(ESkillTaskEndReason::SkillFinished);
	}
}

void FSkillRunner::Finish(ESkillTaskEndReason Reason)
{
	if (bFinished)
	{
		return;
	}
	EndAllActiveSustainTasks(Reason);
	ReleaseAllInstances();
	bFinished = true;
	CurrentStageIndex = INDEX_NONE;
}

// =============================================================
// 外部 API
// =============================================================

void FSkillRunner::SendEvent(FName EventName, ESkillEventTiming Timing)
{
	if (bFinished || EventName.IsNone())
	{
		return;
	}

	FPendingEvent Event{EventName};
	switch (Timing)
	{
	case ESkillEventTiming::Immediate:      ImmediateQueue.Add(Event); break;
	case ESkillEventTiming::FrameEnd:       FrameEndQueue.Add(Event); break;
	case ESkillEventTiming::NextFrameBegin: NextFrameBeginQueue.Add(Event); break;
	}
}

void FSkillRunner::RequestStageChange(int32 TargetStageIndex, EStageChangeReason Reason, int32 Priority)
{
	if (bFinished || !Skill)
	{
		return;
	}
	if (!Skill->Stages.IsValidIndex(TargetStageIndex))
	{
		UE_LOG(LogSkillRunner, Warning, TEXT("阶段切换请求非法：索引 %d"), TargetStageIndex);
		return;
	}

	if (USkillStage* CurStage = GetCurrentStage())
	{
		if (!CurStage->bAllowStageChange)
		{
			return;
		}
	}

	if (PendingStageChange.IsSet() && PendingStageChange->Priority > Priority)
	{
		return;
	}

	FStageChangeRequest Req;
	Req.TargetStageIndex = TargetStageIndex;
	Req.Reason = Reason;
	Req.Priority = Priority;
	PendingStageChange = Req;
}

void FSkillRunner::RequestEndSkill()
{
	bRequestEnd = true;
}

// =============================================================
// 运行时内存访问
// =============================================================

void* FSkillRunner::GetSkillDataRaw() const
{
	if (!Skill || RuntimeMemory.Num() == 0) return nullptr;
	const FSkillRuntimeLayout& L = Skill->CompiledLayout;
	if (L.SkillDataSize <= 0) return nullptr;
	return const_cast<uint8*>(RuntimeMemory.GetData()) + L.SkillDataOffset;
}

void* FSkillRunner::GetStageDataRaw(int32 StageIndex) const
{
	if (!Skill || RuntimeMemory.Num() == 0) return nullptr;
	const FSkillRuntimeLayout& L = Skill->CompiledLayout;
	if (!L.StageLayouts.IsValidIndex(StageIndex)) return nullptr;
	const FSkillStageDataLayout& S = L.StageLayouts[StageIndex];
	if (S.StageDataSize <= 0) return nullptr;
	return const_cast<uint8*>(RuntimeMemory.GetData()) + S.StageDataOffset;
}

void* FSkillRunner::GetTaskDataRaw(int32 StageIndex, int32 TimelineIndex) const
{
	if (!Skill || RuntimeMemory.Num() == 0) return nullptr;
	const FSkillRuntimeLayout& L = Skill->CompiledLayout;
	if (!L.StageLayouts.IsValidIndex(StageIndex)) return nullptr;
	const FSkillStageDataLayout& S = L.StageLayouts[StageIndex];
	if (!S.TaskLayouts.IsValidIndex(TimelineIndex)) return nullptr;
	const FSkillTaskDataLayout& T = S.TaskLayouts[TimelineIndex];
	if (T.DataSize <= 0) return nullptr;
	return const_cast<uint8*>(RuntimeMemory.GetData()) + T.DataOffset;
}

// =============================================================
// Tick 阶段
// =============================================================

void FSkillRunner::Tick_FrameBegin()
{
	if (NextFrameBeginQueue.Num() == 0) return;
	TArray<FPendingEvent> Drained = MoveTemp(NextFrameBeginQueue);
	NextFrameBeginQueue.Reset();
	for (const FPendingEvent& Event : Drained)
	{
		ImmediateQueue.Add(Event);
	}
	Tick_DrainImmediateQueue();
}

void FSkillRunner::Tick_AdvanceTimeline(float DeltaTime)
{
	USkillStage* Stage = GetCurrentStage();
	if (!Stage) return;

	const float NewSkillTime  = CurrentSkillTime + DeltaTime;
	const float NewStageTime  = CurrentStageTime + DeltaTime;

	// 1) 触发已跨过 StartTime 的瞬间任务。
	while (NextInstantCursor < Stage->Timeline.Num())
	{
		const FSkillTimelineTask& Entry = Stage->Timeline[NextInstantCursor];
		if (!Entry.Task) { ++NextInstantCursor; continue; }

		if (Entry.StartTime > NewStageTime)
		{
			break;
		}

		if (!Entry.Task->IsSustainTask())
		{
			ExecuteInstantTimelineTask(NextInstantCursor);
		}
		++NextInstantCursor;
	}

	// 2) 管理持续任务。每帧扫描所有时间轴条目，对常规阶段大小开销小，后续可用有序索引优化。
	for (int32 i = 0; i < Stage->Timeline.Num(); ++i)
	{
		const FSkillTimelineTask& Entry = Stage->Timeline[i];
		if (!Entry.Task || !Entry.Task->IsSustainTask()) continue;

		const float StartT = Entry.StartTime;
		const float EndT   = (Entry.EndTime > Entry.StartTime) ? Entry.EndTime : Entry.StartTime;

		// 查找或添加活跍记录。
		FActiveSustain* Active = ActiveSustainTasks.FindByPredicate(
			[i](const FActiveSustain& A) { return A.TimelineIndex == i; });

		const bool bShouldBeRunning = (NewStageTime >= StartT && NewStageTime < EndT);

		if (bShouldBeRunning)
		{
			if (!Active)
			{
				FActiveSustain New;
				New.TimelineIndex = i;
				ActiveSustainTasks.Add(New);
				Active = &ActiveSustainTasks.Last();
			}
			if (!Active->bStarted)
			{
				StartSustain(i);
				Active->bStarted = true;
			}
			else
			{
				TickSustain(i, DeltaTime);
			}
		}
		else if (Active && Active->bStarted && !Active->bEnded && NewStageTime >= EndT)
		{
			EndSustain(i, ESkillTaskEndReason::NaturalEnd);
			Active->bEnded = true;
		}
	}

	// 删除已结束的记录。
	ActiveSustainTasks.RemoveAll([](const FActiveSustain& A) { return A.bEnded; });

	CurrentSkillTime = NewSkillTime;
	CurrentStageTime = NewStageTime;

	// 阶段时长届满时自动进入下一阶段。
	if (CurrentStageTime >= Stage->Duration && !PendingStageChange.IsSet())
	{
		const int32 Next = Stage->DefaultNextStageIndex;
		if (Skill->Stages.IsValidIndex(Next))
		{
			RequestStageChange(Next, EStageChangeReason::Default, /*Priority*/ 0);
		}
		else
		{
			bRequestEnd = true;
		}
	}
}

void FSkillRunner::Tick_DrainImmediateQueue()
{
	// 一条一条处理事件；任务可能推入更多立即事件。
	while (ImmediateQueue.Num() > 0)
	{
		const FPendingEvent Event = ImmediateQueue[0];
		ImmediateQueue.RemoveAt(0);

		USkillStage* Stage = GetCurrentStage();
		USkillTask* Task = nullptr;
		if (Stage)
		{
			Task = Stage->FindEventTask(Event.EventName);
		}
		if (!Task && Skill)
		{
			Task = Skill->FindSkillEventTask(Event.EventName);
		}
		if (Task)
		{
			ExecuteEventTask(Task, Event.EventName);
		}
	}
}

void FSkillRunner::Tick_FrameEnd()
{
	if (FrameEndQueue.Num() == 0) return;
	TArray<FPendingEvent> Drained = MoveTemp(FrameEndQueue);
	FrameEndQueue.Reset();
	for (const FPendingEvent& Event : Drained)
	{
		ImmediateQueue.Add(Event);
	}
	Tick_DrainImmediateQueue();
}

void FSkillRunner::Tick_ProcessStageChange()
{
	if (!PendingStageChange.IsSet()) return;
	const FStageChangeRequest Req = PendingStageChange.GetValue();
	PendingStageChange.Reset();

	ExitCurrentStage(ESkillTaskEndReason::StageChanged);
	EnterStage(Req.TargetStageIndex);
}

// =============================================================
// 辅助函数
// =============================================================

USkillStage* FSkillRunner::GetCurrentStage() const
{
	return GetStage(CurrentStageIndex);
}

USkillStage* FSkillRunner::GetStage(int32 Index) const
{
	if (!Skill || !Skill->Stages.IsValidIndex(Index)) return nullptr;
	return Skill->Stages[Index];
}

void FSkillRunner::EnterStage(int32 NewStageIndex)
{
	CurrentStageIndex = NewStageIndex;
	CurrentStageTime = 0.f;
	NextInstantCursor = 0;
	ActiveSustainTasks.Reset();

	// 清零阶段数据块及其任务数据块以得到干净的初始状态。
	if (Skill && Skill->CompiledLayout.StageLayouts.IsValidIndex(NewStageIndex))
	{
		const FSkillStageDataLayout& S = Skill->CompiledLayout.StageLayouts[NewStageIndex];
		if (S.StageDataSize > 0 && RuntimeMemory.IsValidIndex(S.StageDataOffset))
		{
			FMemory::Memzero(RuntimeMemory.GetData() + S.StageDataOffset, S.StageDataSize);
		}
		for (const FSkillTaskDataLayout& T : S.TaskLayouts)
		{
			if (T.DataSize > 0 && RuntimeMemory.IsValidIndex(T.DataOffset))
			{
				FMemory::Memzero(RuntimeMemory.GetData() + T.DataOffset, T.DataSize);
			}
		}
	}
}

void FSkillRunner::ExitCurrentStage(ESkillTaskEndReason Reason)
{
	EndAllActiveSustainTasks(Reason);
	// 将每阶段的实例化任务归还到对象池。
	for (auto It = ActiveInstances.CreateIterator(); It; ++It)
	{
		TStrongObjectPtr<USkillTask>& Inst = It->Value;
		if (Inst.IsValid())
		{
			Inst->Reset();
			InstancePool.FindOrAdd(Inst->GetClass()).Add(Inst);
		}
	}
	ActiveInstances.Reset();
}

void FSkillRunner::ExecuteEventTask(USkillTask* SourceTask, FName EventName)
{
	if (!SourceTask) return;

	// 事件使用合成时间轴索引 -1（没有每任务数据槽）。
	USkillTask* Task = AcquireTask(SourceTask, CurrentStageIndex, /*TimelineIndex*/ INDEX_NONE);
	if (!Task) return;

	FSkillTaskContext Ctx;
	BuildTaskContext(Ctx, CurrentStageIndex, INDEX_NONE, 0.f);
	Task->OnExecute(Ctx);

	// 对于事件触发的持续任务，目前仅执行 OnExecute。
	// 持续任务通常应配置在时间轴上。
	if (SourceTask->bCreateInstance)
	{
		Task->Reset();
		InstancePool.FindOrAdd(Task->GetClass()).Add(TStrongObjectPtr<USkillTask>(Task));
	}
}

void FSkillRunner::ExecuteInstantTimelineTask(int32 TimelineIndex)
{
	USkillStage* Stage = GetCurrentStage();
	if (!Stage || !Stage->Timeline.IsValidIndex(TimelineIndex)) return;
	USkillTask* Source = Stage->Timeline[TimelineIndex].Task;
	if (!Source) return;

	USkillTask* Task = AcquireTask(Source, CurrentStageIndex, TimelineIndex);
	if (!Task) return;

	FSkillTaskContext Ctx;
	BuildTaskContext(Ctx, CurrentStageIndex, TimelineIndex, 0.f);
	Task->OnExecute(Ctx);

	if (Source->bCreateInstance)
	{
		Task->Reset();
		InstancePool.FindOrAdd(Task->GetClass()).Add(TStrongObjectPtr<USkillTask>(Task));
		ActiveInstances.Remove(TimelineIndex);
	}
}

void FSkillRunner::StartSustain(int32 TimelineIndex)
{
	USkillStage* Stage = GetCurrentStage();
	if (!Stage || !Stage->Timeline.IsValidIndex(TimelineIndex)) return;
	USkillTask* Source = Stage->Timeline[TimelineIndex].Task;
	if (!Source) return;

	USkillTask* Task = AcquireTask(Source, CurrentStageIndex, TimelineIndex);
	if (!Task) return;
	USustainSkillTask* Sustain = Cast<USustainSkillTask>(Task);
	if (!Sustain) return;

	FSkillTaskContext Ctx;
	BuildTaskContext(Ctx, CurrentStageIndex, TimelineIndex, 0.f);
	Sustain->OnStart(Ctx);
}

void FSkillRunner::TickSustain(int32 TimelineIndex, float DeltaTime)
{
	USkillStage* Stage = GetCurrentStage();
	if (!Stage || !Stage->Timeline.IsValidIndex(TimelineIndex)) return;
	USkillTask* Source = Stage->Timeline[TimelineIndex].Task;
	if (!Source) return;

	// 对于实例化任务，Re-acquire 返回相同的池化实例。
	USkillTask* Task = AcquireTask(Source, CurrentStageIndex, TimelineIndex);
	USustainSkillTask* Sustain = Cast<USustainSkillTask>(Task);
	if (!Sustain) return;

	FSkillTaskContext Ctx;
	BuildTaskContext(Ctx, CurrentStageIndex, TimelineIndex, DeltaTime);
	Sustain->OnTick(Ctx, DeltaTime);
}

void FSkillRunner::EndSustain(int32 TimelineIndex, ESkillTaskEndReason Reason)
{
	USkillStage* Stage = GetCurrentStage();
	if (!Stage || !Stage->Timeline.IsValidIndex(TimelineIndex)) return;
	USkillTask* Source = Stage->Timeline[TimelineIndex].Task;
	if (!Source) return;

	USkillTask* Task = AcquireTask(Source, CurrentStageIndex, TimelineIndex);
	USustainSkillTask* Sustain = Cast<USustainSkillTask>(Task);
	if (!Sustain) return;

	FSkillTaskContext Ctx;
	BuildTaskContext(Ctx, CurrentStageIndex, TimelineIndex, 0.f);
	Sustain->OnEnd(Ctx, Reason);

	if (Source->bCreateInstance)
	{
		Sustain->Reset();
		InstancePool.FindOrAdd(Sustain->GetClass()).Add(TStrongObjectPtr<USkillTask>(Sustain));
		ActiveInstances.Remove(TimelineIndex);
	}
}

void FSkillRunner::EndAllActiveSustainTasks(ESkillTaskEndReason Reason)
{
	for (FActiveSustain& A : ActiveSustainTasks)
	{
		if (A.bStarted && !A.bEnded)
		{
			EndSustain(A.TimelineIndex, Reason);
			A.bEnded = true;
		}
	}
	ActiveSustainTasks.Reset();
}

USkillTask* FSkillRunner::AcquireTask(USkillTask* Source, int32 InStageIndex, int32 TimelineIndex)
{
	if (!Source) return nullptr;
	if (!Source->bCreateInstance)
	{
		return Source; // 无状态：直接使用 CDO / 资产共享对象。
	}

	if (TimelineIndex != INDEX_NONE)
	{
		if (TStrongObjectPtr<USkillTask>* Found = ActiveInstances.Find(TimelineIndex))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
		}
	}

	UClass* Class = Source->GetClass();
	TStrongObjectPtr<USkillTask> Instance;
	if (TArray<TStrongObjectPtr<USkillTask>>* Pool = InstancePool.Find(Class))
	{
		if (Pool->Num() > 0)
		{
			Instance = Pool->Pop(/*bAllowShrinking*/ false);
		}
	}
	if (!Instance.IsValid())
	{
		Instance = TStrongObjectPtr<USkillTask>(NewObject<USkillTask>(Component, Class));
	}

	if (TimelineIndex != INDEX_NONE)
	{
		ActiveInstances.Add(TimelineIndex, Instance);
	}
	return Instance.Get();
}

void FSkillRunner::ReleaseAllInstances()
{
	for (auto It = ActiveInstances.CreateIterator(); It; ++It)
	{
		TStrongObjectPtr<USkillTask>& Inst = It->Value;
		if (Inst.IsValid())
		{
			Inst->Reset();
			InstancePool.FindOrAdd(Inst->GetClass()).Add(Inst);
		}
	}
	ActiveInstances.Reset();
}

void FSkillRunner::BuildTaskContext(FSkillTaskContext& OutCtx, int32 InStageIndex, int32 TimelineIndex, float DeltaTime) const
{
	OutCtx.Component = Component;
	OutCtx.Runner = const_cast<FSkillRunner*>(this);
	OutCtx.Skill = Skill;
	OutCtx.StageIndex = InStageIndex;
	OutCtx.TimelineIndex = TimelineIndex;
	OutCtx.CurrentSkillTime = CurrentSkillTime;
	OutCtx.CurrentStageTime = CurrentStageTime;
	OutCtx.DeltaTime = DeltaTime;

	OutCtx.World        = SkillCtx.World;
	OutCtx.OwnerActor   = SkillCtx.OwnerActor;
	OutCtx.AvatarActor  = SkillCtx.AvatarActor;
	OutCtx.SourceObject = SkillCtx.SourceObject;
}
