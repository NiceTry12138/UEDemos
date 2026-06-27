// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class USkillAsset;
class USkillGraph;
class USkillStage;
class USkillTask;
class USkillPreviewConfig;
class IDetailsView;
class SGraphEditor;
class SSkillTimeline;
class SSkillPreviewViewport;

/**
 * USkillAsset 的资产编辑工具套件。
 * 布局：图居中，时间轴停靠底部，视口居右上，属性面板居右下。
 */
class FSkillAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	void InitSkillAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, USkillAsset* InAsset);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	USkillAsset* GetSkillAsset() const { return SkillAsset.Get(); }

	/** 将时间轴更新为指定阶段的内容。 */
	void SetSelectedStage(USkillStage* InStage);

	/** 创建一个新阶段并添加到资产中。 */
	FReply OnAddStageClicked();

protected:
	// Tab 生成回调。
	TSharedRef<class SDockTab> SpawnTab_Graph(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnTab_Timeline(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnTab_Viewport(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnTab_Details(const class FSpawnTabArgs& Args);

	void OnGraphSelectionChanged(const TSet<class UObject*>& NewSelection);
	void OnTimelineTimingChanged();
	void OnTimelineTaskSelected(USkillTask* Task);
	void OnPropertyChanged(UObject* ChangedObject, const struct FPropertyChangedEvent& Evt);

	void RebuildGraph();

private:
	TWeakObjectPtr<USkillAsset>  SkillAsset;
	TWeakObjectPtr<USkillGraph>  SkillGraph;
	TWeakObjectPtr<USkillStage>  SelectedStage;
	TWeakObjectPtr<USkillTask>   SelectedTask;

	/** 临时预览 Actor 配置对象，在 InitSkillAssetEditor 中 AddToRoot 以防止 GC。 */
	USkillPreviewConfig* PreviewConfig = nullptr;

	TSharedPtr<SGraphEditor>          GraphEditor;
	TSharedPtr<SSkillTimeline>        Timeline;
	TSharedPtr<SSkillPreviewViewport> Viewport;

	// 属性面板： 4 个独立视图以可折叠区域呈现。
	TSharedPtr<IDetailsView> DetailsView_Asset;     // 分类 1 – 技能资产
	TSharedPtr<IDetailsView> DetailsView_Stage;     // 分类 2 – 技能阶段
	TSharedPtr<IDetailsView> DetailsView_Task;      // 分类 3 – 技能任务
	TSharedPtr<IDetailsView> DetailsView_Viewport;  // 分类 4 – 视口配置

	FDelegateHandle PropertyChangedHandle;
};
