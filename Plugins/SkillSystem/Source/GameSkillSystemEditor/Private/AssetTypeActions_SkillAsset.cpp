// Copyright. GameSkillSystemEditor.
#include "AssetTypeActions_SkillAsset.h"
#include "SkillAsset.h"
#include "SkillAssetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_SkillAsset"

FText FAssetTypeActions_SkillAsset::GetName() const
{
	return LOCTEXT("Name", "Skill Asset");
}

UClass* FAssetTypeActions_SkillAsset::GetSupportedClass() const
{
	return USkillAsset::StaticClass();
}

void FAssetTypeActions_SkillAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UObject* Obj : InObjects)
	{
		if (USkillAsset* SkillAsset = Cast<USkillAsset>(Obj))
		{
			TSharedRef<FSkillAssetEditorToolkit> NewEditor(new FSkillAssetEditorToolkit());
			NewEditor->InitSkillAssetEditor(Mode, EditWithinLevelEditor, SkillAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
