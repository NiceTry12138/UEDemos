// Copyright. GameSkillSystemEditor.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SkillAssetFactory.generated.h"

UCLASS()
class USkillAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	USkillAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
};
