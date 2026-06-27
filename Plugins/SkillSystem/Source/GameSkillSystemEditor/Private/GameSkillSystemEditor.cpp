// Copyright. GameSkillSystemEditor.
#include "GameSkillSystemEditor.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"

#include "AssetTypeActions_SkillAsset.h"
#include "Graph/SkillGraphNodeFactory.h"

#define LOCTEXT_NAMESPACE "FGameSkillSystemEditorModule"

void FGameSkillSystemEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	SkillAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("GameSkill")),
		LOCTEXT("GameSkillCategory", "Game Skill"));

	auto Action = MakeShared<FAssetTypeActions_SkillAsset>(SkillAssetCategory);
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetActions.Add(Action);

	NodeFactory = MakeShared<FSkillGraphNodeFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(NodeFactory);
}

void FGameSkillSystemEditorModule::ShutdownModule()
{
	if (NodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(NodeFactory);
		NodeFactory.Reset();
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetActions)
		{
			if (Action.IsValid())
			{
				AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
			}
		}
	}
	RegisteredAssetActions.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameSkillSystemEditorModule, GameSkillSystemEditor)
