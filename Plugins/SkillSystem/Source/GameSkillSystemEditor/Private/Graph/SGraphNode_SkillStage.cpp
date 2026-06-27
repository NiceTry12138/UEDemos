// Copyright. GameSkillSystemEditor.
#include "Graph/SGraphNode_SkillStage.h"
#include "Graph/SkillGraphNodes.h"
#include "SkillStage.h"
#include "SkillTask.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"

void SGraphNode_SkillStage::Construct(const FArguments& InArgs, USkillGraphNode_Stage* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SGraphNode_SkillStage::UpdateGraphNode()
{
	InputPins.Reset();
	OutputPins.Reset();
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

	this->GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Graph.Node.Body"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			// 标题
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				SNew(STextBlock)
				.Text(this, &SGraphNode_SkillStage::GetNodeTitleText)
				.Font(FAppStyle::GetFontStyle("Graph.Node.NodeTitle"))
			]
			// 引脚行
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).FillWidth(1.f)
				[
					SAssignNew(LeftNodeBox, SVerticalBox)
				]
				+ SHorizontalBox::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).FillWidth(1.f)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
			]
			// 任务列表
			+ SVerticalBox::Slot().AutoHeight().Padding(4)
			[
				BuildTaskList()
			]
		]
	];

	CreatePinWidgets();
}

void SGraphNode_SkillStage::CreatePinWidgets()
{
	if (!GraphNode) return;
	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (!Pin || Pin->bHidden) continue;
		TSharedPtr<SGraphPin> PinWidget = SNew(SGraphPin, Pin);
		AddPin(PinWidget.ToSharedRef());
	}
}

FText SGraphNode_SkillStage::GetNodeTitleText() const
{
	return GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
}

TSharedRef<SWidget> SGraphNode_SkillStage::BuildTaskList()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	USkillGraphNode_Stage* StageNode = Cast<USkillGraphNode_Stage>(GraphNode);
	if (!StageNode || !StageNode->Stage.IsValid()) return Box;

	USkillStage* Stage = StageNode->Stage.Get();

	// 按 StartTime 构建排序索引列表。
	TArray<int32> Indices;
	Indices.Reserve(Stage->Timeline.Num());
	for (int32 i = 0; i < Stage->Timeline.Num(); ++i) Indices.Add(i);
	Indices.Sort([Stage](int32 A, int32 B)
	{
		return Stage->Timeline[A].StartTime < Stage->Timeline[B].StartTime;
	});

	for (int32 Index : Indices)
	{
		const FSkillTimelineTask& Entry = Stage->Timeline[Index];
		FText TaskName = Entry.Task ? Entry.Task->GetDisplayNameText() : FText::FromString(TEXT("<None>"));

		Box->AddSlot().AutoHeight().Padding(2)
		[
			SNew(STextBlock).Text(TaskName)
		];
	}
	return Box;
}
