// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "AssetTypeCategories.h"

class IAssetTypeActions;

class FGameSkillSystemEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	EAssetTypeCategories::Type GetSkillAssetCategory() const { return SkillAssetCategory; }

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetActions;
	EAssetTypeCategories::Type SkillAssetCategory = EAssetTypeCategories::Misc;

	TSharedPtr<class FSkillGraphNodeFactory> NodeFactory;
};
