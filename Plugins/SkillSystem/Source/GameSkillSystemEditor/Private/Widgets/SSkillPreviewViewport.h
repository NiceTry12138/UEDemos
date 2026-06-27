// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "PreviewScene.h"

class FSkillPreviewViewportClient;
class USkillAsset;
class USkillPreviewConfig;
class AActor;

/**
 * 技能编辑器用的预览视口。
 * 拥有自身的 FPreviewScene + 视口客户端，
 * 并根据 USkillAsset::PreviewActorClass 保持一个预览 Actor。
 */
class SSkillPreviewViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SSkillPreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, USkillAsset* InAsset);

	void RefreshPreviewActor();
	void RefreshFromConfig(USkillPreviewConfig* Config);

	UWorld* GetPreviewWorld() const;
	AActor* GetPreviewActor() const { return PreviewActor.Get(); }

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual void BindCommands() override;

private:
	TSharedPtr<FPreviewScene> PreviewScene;
	TSharedPtr<FSkillPreviewViewportClient> ViewportClient;

	TWeakObjectPtr<USkillAsset> AssetWeak;
	TWeakObjectPtr<AActor> PreviewActor;

	/** 从 USkillPreviewConfig::PreviewActors 生成的 Actor 列表。 */
	TArray<TWeakObjectPtr<AActor>> UserSpawnedActors;
};

class FSkillPreviewViewportClient : public FEditorViewportClient
{
public:
	FSkillPreviewViewportClient(const TSharedRef<SSkillPreviewViewport>& InViewport, FPreviewScene* InPreviewScene);
};
