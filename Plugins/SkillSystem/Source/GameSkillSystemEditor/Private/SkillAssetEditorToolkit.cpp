// Copyright. GameSkillSystemEditor.
#include "SkillAssetEditorToolkit.h"

#include "SkillAsset.h"
#include "SkillStage.h"
#include "SkillTask.h"
#include "SkillPreviewConfig.h"

#include "Graph/SkillGraph.h"
#include "Graph/SkillGraphNodes.h"
#include "Graph/SkillGraphSchema.h"

#include "Widgets/SSkillTimeline.h"
#include "Widgets/SSkillPreviewViewport.h"

#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "EdGraphUtilities.h"

#define LOCTEXT_NAMESPACE "SkillAssetEditor"

namespace SkillEditorTabs
{
	static const FName Graph    = FName(TEXT("SkillEditor_Graph"));
	static const FName Timeline = FName(TEXT("SkillEditor_Timeline"));
	static const FName Viewport = FName(TEXT("SkillEditor_Viewport"));
	static const FName Details  = FName(TEXT("SkillEditor_Details"));
}

void FSkillAssetEditorToolkit::InitSkillAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, USkillAsset* InAsset)
{
	SkillAsset = InAsset;

	// 构建（或复用）资产持有的图对象（临时）。
	if (USkillGraph* Existing = Cast<USkillGraph>(StaticFindObjectFast(USkillGraph::StaticClass(), InAsset, TEXT("SkillGraph"))))
	{
		SkillGraph = Existing;
	}
	else
	{
		USkillGraph* NewGraph = NewObject<USkillGraph>(InAsset, TEXT("SkillGraph"), RF_Transient);
		NewGraph->Schema = USkillGraphSchema::StaticClass();
		NewGraph->OwningAsset = InAsset;
		SkillGraph = NewGraph;
	}
	if (SkillGraph.IsValid())
	{
		SkillGraph->RebuildFromAsset();
	}

	// 构建 4 个属性视图（每个分类区域一个）。
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bShowOptions = false;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	Args.bAllowSearch = false;

	DetailsView_Asset    = PropertyModule.CreateDetailView(Args);
	DetailsView_Stage    = PropertyModule.CreateDetailView(Args);
	DetailsView_Task     = PropertyModule.CreateDetailView(Args);
	DetailsView_Viewport = PropertyModule.CreateDetailView(Args);

	DetailsView_Asset->SetObject(InAsset);
	// 阶段和任务在用户选择之前初始为空。

	// 创建临时视口配置并 Root 它以防止 GC 回收。
	PreviewConfig = NewObject<USkillPreviewConfig>(GetTransientPackage(), NAME_None, RF_Transient);
	PreviewConfig->AddToRoot();
	DetailsView_Viewport->SetObject(PreviewConfig);

	// 属性变更转发器，保持时间轴 / 图与资产同步。
	PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
		[WeakThis = TWeakPtr<FSkillAssetEditorToolkit>(SharedThis(this))](UObject* Obj, FPropertyChangedEvent& Evt)
		{
			if (TSharedPtr<FSkillAssetEditorToolkit> Pinned = WeakThis.Pin())
			{
				Pinned->OnPropertyChanged(Obj, Evt);
			}
		});

	const TSharedRef<FTabManager::FLayout> StandaloneLayout = FTabManager::NewLayout("Standalone_SkillEditor_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			// 中层列：图（上）+ 时间轴（下）
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.65f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(SkillEditorTabs::Graph, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.35f)
					->AddTab(SkillEditorTabs::Timeline, ETabState::OpenedTab)
					->SetHideTabWell(false)
				)
			)
			// 右列：视口 + 属性
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.35f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(SkillEditorTabs::Viewport, ETabState::OpenedTab)
					->SetHideTabWell(false)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(SkillEditorTabs::Details, ETabState::OpenedTab)
					->SetHideTabWell(false)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		Mode, InitToolkitHost,
		FName(TEXT("SkillAssetEditor")),
		StandaloneLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar,
		InAsset);
}

FName FSkillAssetEditorToolkit::GetToolkitFName() const          { return FName("SkillAssetEditor"); }
FText FSkillAssetEditorToolkit::GetBaseToolkitName() const       { return LOCTEXT("AppLabel", "Skill Asset Editor"); }
FString FSkillAssetEditorToolkit::GetWorldCentricTabPrefix() const { return TEXT("Skill "); }
FLinearColor FSkillAssetEditorToolkit::GetWorldCentricTabColorScale() const { return FLinearColor(0.2f, 0.4f, 0.8f, 0.5f); }

void FSkillAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "Skill Asset Editor"));
	auto Cat = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(SkillEditorTabs::Graph, FOnSpawnTab::CreateSP(this, &FSkillAssetEditorToolkit::SpawnTab_Graph))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph")).SetGroup(Cat);

	InTabManager->RegisterTabSpawner(SkillEditorTabs::Timeline, FOnSpawnTab::CreateSP(this, &FSkillAssetEditorToolkit::SpawnTab_Timeline))
		.SetDisplayName(LOCTEXT("TimelineTab", "Timeline")).SetGroup(Cat);

	InTabManager->RegisterTabSpawner(SkillEditorTabs::Viewport, FOnSpawnTab::CreateSP(this, &FSkillAssetEditorToolkit::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("ViewportTab", "Preview")).SetGroup(Cat);

	InTabManager->RegisterTabSpawner(SkillEditorTabs::Details, FOnSpawnTab::CreateSP(this, &FSkillAssetEditorToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details")).SetGroup(Cat);
}

void FSkillAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(SkillEditorTabs::Graph);
	InTabManager->UnregisterTabSpawner(SkillEditorTabs::Timeline);
	InTabManager->UnregisterTabSpawner(SkillEditorTabs::Viewport);
	InTabManager->UnregisterTabSpawner(SkillEditorTabs::Details);

	if (PropertyChangedHandle.IsValid())
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
		PropertyChangedHandle.Reset();
	}

	// 释放临时配置对象的 GC 根引用。
	if (PreviewConfig)
	{
		PreviewConfig->RemoveFromRoot();
		PreviewConfig = nullptr;
	}
}

// --- 图 Tab -----------------------------------------------------------

TSharedRef<SDockTab> FSkillAssetEditorToolkit::SpawnTab_Graph(const FSpawnTabArgs& Args)
{
	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FSkillAssetEditorToolkit::OnGraphSelectionChanged);

	GraphEditor = SNew(SGraphEditor)
		.GraphToEdit(SkillGraph.Get())
		.IsEditable(false)
		.GraphEvents(Events);

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabLabel", "Graph"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddStage", "+ Add Stage"))
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply { return OnAddStageClicked(); }))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				GraphEditor.ToSharedRef()
			]
		];
}

// --- 时间轴 Tab --------------------------------------------------------

TSharedRef<SDockTab> FSkillAssetEditorToolkit::SpawnTab_Timeline(const FSpawnTabArgs& Args)
{
	Timeline = SNew(SSkillTimeline)
		.OnTimingChanged(SSkillTimeline::FOnTimingChanged::CreateSP(this, &FSkillAssetEditorToolkit::OnTimelineTimingChanged))
		.OnTaskSelected(SSkillTimeline::FOnTaskSelected::CreateSP(this, &FSkillAssetEditorToolkit::OnTimelineTaskSelected));

	// 默认展示第 0 个阶段（如果存在）。
	if (USkillAsset* Asset = SkillAsset.Get())
	{
		if (Asset->Stages.Num() > 0)
		{
			SelectedStage = Asset->Stages[0];
			Timeline->SetStage(Asset->Stages[0]);
		}
	}

	return SNew(SDockTab)
		.Label(LOCTEXT("TimelineTabLabel", "Timeline"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f)
			[
				SNew(STextBlock)
				.Margin(FMargin(6.f, 2.f))
				.Text_Lambda([this]() -> FText
				{
					return SelectedStage.IsValid()
						? SelectedStage->GetDisplayNameText()
						: LOCTEXT("NoStageSelected", "(no stage selected)");
				})
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				Timeline.ToSharedRef()
			]
		];
}

// --- 视口 Tab --------------------------------------------------------

TSharedRef<SDockTab> FSkillAssetEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	Viewport = SNew(SSkillPreviewViewport, SkillAsset.Get());
	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTabLabel", "Preview"))
		[
			Viewport.ToSharedRef()
		];
}

// --- 属性 Tab ---------------------------------------------------------

TSharedRef<SDockTab> FSkillAssetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	// 构建带标题的可折叠区域的辅助 lambda。
	auto MakeSection = [](FText Title, TSharedRef<SWidget> Content) -> TSharedRef<SWidget>
	{
		return SNew(SExpandableArea)
			.AreaTitle(Title)
			.InitiallyCollapsed(false)
			.Padding(FMargin(0.f, 2.f))
			.BodyContent() [ Content ];
	};

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				MakeSection(LOCTEXT("AssetSection",    "Skill Asset"),    DetailsView_Asset.ToSharedRef())
			]
			+ SScrollBox::Slot()
			[
				MakeSection(LOCTEXT("StageSection",    "Skill Stage"),    DetailsView_Stage.ToSharedRef())
			]
			+ SScrollBox::Slot()
			[
				MakeSection(LOCTEXT("TaskSection",     "Skill Task"),     DetailsView_Task.ToSharedRef())
			]
			+ SScrollBox::Slot()
			[
				MakeSection(LOCTEXT("ViewportSection", "Viewport Config"), DetailsView_Viewport.ToSharedRef())
			]
		];
}

// --- 选中 / 变更处理器 -----------------------------------------

void FSkillAssetEditorToolkit::OnGraphSelectionChanged(const TSet<UObject*>& NewSelection)
{
	for (UObject* Obj : NewSelection)
	{
		if (USkillGraphNode_Stage* StageNode = Cast<USkillGraphNode_Stage>(Obj))
		{
			SelectedStage = StageNode->Stage.Get();
			SetSelectedStage(StageNode->Stage.Get());
			DetailsView_Stage->SetObject(StageNode->Stage.Get());
			return;
		}
	}
	// 没有相关选择：清空阶段属性视图。
	DetailsView_Stage->SetObject(nullptr);
}

void FSkillAssetEditorToolkit::SetSelectedStage(USkillStage* InStage)
{
	if (Timeline.IsValid())
	{
		Timeline->SetStage(InStage);
	}
}

void FSkillAssetEditorToolkit::OnTimelineTimingChanged()
{
	if (USkillAsset* Asset = SkillAsset.Get())
	{
		Asset->Modify();
		Asset->Compile();
	}
}

void FSkillAssetEditorToolkit::OnPropertyChanged(UObject* ChangedObject, const FPropertyChangedEvent& Evt)
{
	if (ChangedObject == PreviewConfig)
	{
		// 仅刷新视口 Actor，无需重建图。
		if (Viewport.IsValid()) Viewport->RefreshFromConfig(PreviewConfig);
		return;
	}
	RebuildGraph();
	if (Viewport.IsValid())
	{
		Viewport->RefreshPreviewActor();
	}
}

void FSkillAssetEditorToolkit::RebuildGraph()
{
	if (SkillGraph.IsValid())
	{
		SkillGraph->RebuildFromAsset();
	}
	if (GraphEditor.IsValid())
	{
		GraphEditor->NotifyGraphChanged();
	}
}

// --- 新建阶段 -----------------------------------------------------------

FReply FSkillAssetEditorToolkit::OnAddStageClicked()
{
	USkillAsset* Asset = SkillAsset.Get();
	if (!Asset) return FReply::Handled();

	Asset->Modify();
	USkillStage* NewStage = NewObject<USkillStage>(Asset, NAME_None, RF_Transactional);
	NewStage->StageName = FName(*FString::Printf(TEXT("Stage_%d"), Asset->Stages.Num()));
	NewStage->DisplayName = FText::FromString(FString::Printf(TEXT("Stage %d"), Asset->Stages.Num()));
	NewStage->Duration = 1.f;
	Asset->Stages.Add(NewStage);
	Asset->Compile();

	SelectedStage = NewStage;
	RebuildGraph();

	if (Timeline.IsValid()) Timeline->SetStage(NewStage);
	DetailsView_Stage->SetObject(NewStage);

	return FReply::Handled();
}

// --- 在时间轴中选择任务 ------------------------------------------

void FSkillAssetEditorToolkit::OnTimelineTaskSelected(USkillTask* Task)
{
	SelectedTask = Task;
	DetailsView_Task->SetObject(Task);
}

#undef LOCTEXT_NAMESPACE
