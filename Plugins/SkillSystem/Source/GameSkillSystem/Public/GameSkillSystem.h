// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FGameSkillSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface 接口实现 */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
