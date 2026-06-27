// Copyright. GameSkillSystemEditor.
using UnrealBuildTool;

public class GameSkillSystemEditor : ModuleRules
{
	public GameSkillSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameSkillSystem",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"EditorWidgets",
			"UnrealEd",
			"AssetTools",
			"GraphEditor",
			"PropertyEditor",
			"ToolMenus",
			"Kismet",
			"KismetWidgets",
			"AdvancedPreviewScene",
			"ApplicationCore",
			"WorkspaceMenuStructure",
			"Projects",
		});
	}
}
