// Copyright. GameSkillSystemEditor.
#include "SkillAssetFactory.h"
#include "SkillAsset.h"

USkillAssetFactory::USkillAssetFactory()
{
	SupportedClass   = USkillAsset::StaticClass();
	bCreateNew       = true;
	bEditAfterNew    = true;
}

UObject* USkillAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	USkillAsset* NewAsset = NewObject<USkillAsset>(InParent, InClass, InName, Flags);
	return NewAsset;
}
