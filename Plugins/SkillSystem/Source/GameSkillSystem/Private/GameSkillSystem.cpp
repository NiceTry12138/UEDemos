// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSkillSystem.h"

#define LOCTEXT_NAMESPACE "FGameSkillSystemModule"

void FGameSkillSystemModule::StartupModule()
{
	// 此代码在模块加载到内存后执行；具体时机由 .uplugin 文件中每模块的配置决定。
	
}

void FGameSkillSystemModule::ShutdownModule()
{
	// 模块卸载前调用此函数以清理资源。
	// 对于支持动态重载的模块，在卸载前调用。
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameSkillSystemModule, GameSkillSystem)