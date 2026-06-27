// Copyright. GameSkillSystem.
#include "SkillComponent.h"
#include "SkillRunner.h"
#include "SkillAsset.h"

USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 i = Runners.Num() - 1; i >= 0; --i)
	{
		TUniquePtr<FSkillRunner>& Runner = Runners[i];
		if (!Runner.IsValid() || Runner->IsFinished())
		{
			Runners.RemoveAt(i);
			continue;
		}
		Runner->Tick(DeltaTime);
		if (Runner->IsFinished())
		{
			Runners.RemoveAt(i);
		}
	}
}

void USkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TUniquePtr<FSkillRunner>& Runner : Runners)
	{
		if (Runner.IsValid())
		{
			Runner->Finish(ESkillTaskEndReason::OwnerDestroyed);
		}
	}
	Runners.Reset();
	Super::EndPlay(EndPlayReason);
}

bool USkillComponent::ActivateSkill(USkillAsset* SkillAsset)
{
	if (!SkillAsset) return false;

	FSkillContext Ctx;
	Ctx.World        = GetWorld();
	Ctx.OwnerActor   = GetOwner();
	Ctx.AvatarActor  = AvatarActorOverride ? AvatarActorOverride.Get() : GetOwner();
	Ctx.SourceObject = this;
	return ActivateSkillWithContext(SkillAsset, Ctx);
}

bool USkillComponent::ActivateSkillWithContext(USkillAsset* SkillAsset, const FSkillContext& InContext)
{
	if (!SkillAsset) return false;

	TUniquePtr<FSkillRunner> Runner = MakeUnique<FSkillRunner>();
	Runner->SetSkillContext(InContext);
	if (!Runner->Initialize(this, SkillAsset))
	{
		return false;
	}
	Runners.Add(MoveTemp(Runner));
	return true;
}

void USkillComponent::CancelAllSkills()
{
	for (TUniquePtr<FSkillRunner>& Runner : Runners)
	{
		if (Runner.IsValid())
		{
			Runner->Finish(ESkillTaskEndReason::SkillInterrupted);
		}
	}
	Runners.Reset();
}

void USkillComponent::CancelSkill(USkillAsset* SkillAsset)
{
	if (!SkillAsset) return;
	for (int32 i = Runners.Num() - 1; i >= 0; --i)
	{
		if (Runners[i].IsValid() && Runners[i]->GetSkill() == SkillAsset)
		{
			Runners[i]->Finish(ESkillTaskEndReason::SkillInterrupted);
			Runners.RemoveAt(i);
			return;
		}
	}
}

void USkillComponent::SendEventToAll(FName EventName, ESkillEventTiming Timing)
{
	for (TUniquePtr<FSkillRunner>& Runner : Runners)
	{
		if (Runner.IsValid())
		{
			Runner->SendEvent(EventName, Timing);
		}
	}
}
