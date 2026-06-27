// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameSkillSystemBPLibrary.generated.h"

/*
*	蓝图函数库类。
*	其中的每个函数都应为 static，代表可在任意蓝图中调用的节点。
*
*	声明函数时可通过 meta 定义节点元数据，常用说明符如下：
*	BlueprintPure      - 函数不影响宿主对象，生成无执行引脚的纯节点。
*	BlueprintCallable  - 函数可在蓝图中执行，生成带执行引脚的节点。
*	DisplayName        - 节点全名，鼠标悬停及蓝图下拉菜单中显示，可使用 C++ 标识符不允许的字符。
*	CompactNodeTitle   - 节点上显示的简短文字。
*	Keywords           - 用于蓝图下拉搜索的关键词列表，例如 "Print String" 也可通过 "log" 找到。
*	Category           - 节点在蓝图下拉菜单中所属的分类。
*
*	自定义蓝图节点的详细文档请参考：
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class UGameSkillSystemBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "GameSkillSystem sample test testing"), Category = "GameSkillSystemTesting")
	static float GameSkillSystemSampleFunction(float Param);
};
