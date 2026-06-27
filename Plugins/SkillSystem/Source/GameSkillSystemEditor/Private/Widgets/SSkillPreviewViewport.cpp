// Copyright. GameSkillSystemEditor.
#include "Widgets/SSkillPreviewViewport.h"
#include "SkillAsset.h"
#include "SkillPreviewConfig.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "PreviewScene.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/StaticMesh.h"

FSkillPreviewViewportClient::FSkillPreviewViewportClient(const TSharedRef<SSkillPreviewViewport>& InViewport, FPreviewScene* InPreviewScene)
	: FEditorViewportClient(nullptr, InPreviewScene, StaticCastSharedRef<SEditorViewport>(InViewport))
{
	bSetListenerPosition = false;
	SetRealtime(true);
	SetViewLocation(FVector(300, 300, 200));
	SetViewRotation(FRotator(-20, -135, 0));
}

void SSkillPreviewViewport::Construct(const FArguments& InArgs, USkillAsset* InAsset)
{
	AssetWeak = InAsset;
	PreviewScene = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues().SetCreatePhysicsScene(true));

	// --- 地面：使用 Cube，缩放 (10, 10, 1)，Z=-50 使顶面与 Z=0 对齐 ---
	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		UStaticMeshComponent* FloorComp = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
		FloorComp->SetStaticMesh(CubeMesh);
		PreviewScene->AddComponent(FloorComp,
			FTransform(FQuat::Identity, FVector(0.f, 0.f, -50.f), FVector(10.f, 10.f, 1.f)));
	}

	// --- 天空光（环境光）---
	{
		USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
		SkyLight->Intensity = 1.f;
		PreviewScene->AddComponent(SkyLight, FTransform::Identity);
	}

	SEditorViewport::Construct(SEditorViewport::FArguments());
	RefreshPreviewActor();
}

UWorld* SSkillPreviewViewport::GetPreviewWorld() const
{
	return PreviewScene.IsValid() ? PreviewScene->GetWorld() : nullptr;
}

void SSkillPreviewViewport::RefreshPreviewActor()
{
	if (PreviewActor.IsValid())
	{
		PreviewActor->Destroy();
		PreviewActor.Reset();
	}

	USkillAsset* Asset = AssetWeak.Get();
	UWorld* World = GetPreviewWorld();
	if (!Asset || !World || !Asset->PreviewActorClass) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AActor* Spawned = World->SpawnActor<AActor>(Asset->PreviewActorClass, FTransform::Identity, Params))
	{
		PreviewActor = Spawned;
	}
}

TSharedRef<FEditorViewportClient> SSkillPreviewViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShared<FSkillPreviewViewportClient>(SharedThis(this), PreviewScene.Get());
	return ViewportClient.ToSharedRef();
}

void SSkillPreviewViewport::BindCommands()
{
	SEditorViewport::BindCommands();
}

void SSkillPreviewViewport::RefreshFromConfig(USkillPreviewConfig* Config)
{
	UWorld* World = GetPreviewWorld();

	// 销毁之前由用户配置生成的 Actor。
	for (TWeakObjectPtr<AActor>& WeakActor : UserSpawnedActors)
	{
		if (AActor* A = WeakActor.Get()) A->Destroy();
	}
	UserSpawnedActors.Empty();

	if (!Config || !World) return;

	for (const FSkillPreviewActorEntry& Entry : Config->PreviewActors)
	{
		if (!Entry.ActorClass) continue;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform T(Entry.Rotation, Entry.Location, Entry.Scale3D);
		if (AActor* A = World->SpawnActor<AActor>(Entry.ActorClass, T, Params))
		{
			UserSpawnedActors.Add(A);
		}
	}
}
