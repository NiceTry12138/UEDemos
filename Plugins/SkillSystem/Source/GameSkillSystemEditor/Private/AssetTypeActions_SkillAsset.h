// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "AssetTypeCategories.h"

class FAssetTypeActions_SkillAsset : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_SkillAsset(EAssetTypeCategories::Type InCategory) : Category(InCategory) {}

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override { return FColor(60, 140, 255); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override { return Category; }

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

private:
	EAssetTypeCategories::Type Category;
};
