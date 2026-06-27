// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "SkillPreviewConfig.generated.h"

/**
 * 在技能编辑器预览视口中待生成的单个 Actor 条目。
 */
USTRUCT()
struct FSkillPreviewActorEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Preview", meta = (DisplayName = "Actor Class"))
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, Category = "Preview")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Preview")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Preview")
	FVector Scale3D = FVector(1.f, 1.f, 1.f);
};

/**
 * 临时编辑器专用对象，指定在技能编辑器预览视口中生成哪些 Actor。
 * 由 FSkillAssetEditorToolkit 持有（AddToRoot）。
 */
UCLASS()
class USkillPreviewConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Viewport Preview", meta = (DisplayName = "Preview Actors"))
	TArray<FSkillPreviewActorEntry> PreviewActors;
};
