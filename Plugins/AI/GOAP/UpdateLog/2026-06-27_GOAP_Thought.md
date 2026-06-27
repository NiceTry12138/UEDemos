# 2026-06-27 GOAP 思考记录

修改内容: 新增 GOAP 算法说明、代码框架、计算过程、UE 插件实现方案与 SVG 图解。

## 记录

- 创建 `Thought/GOAP_Study_2026-06-27.md`。
- 创建 `Thought/GOAP_Algorithm_Flow.svg`。
- 本次只新增阅读、讨论、思考记录，不编写 GOAP 功能代码。
- 后续正式功能代码应放入 `Plugins/AI/GOAP` 插件内，不能放入 `Empty_54/Source`。

- 扩写 Forward A* 计算过程: OpenSet、GScore、HScore、FScore、ParentAction 回溯、状态去重与更低 GScore 更新、OpenSet 为空的失败条件、GOAP 与 A* 的映射关系。
- 修正项目技术栈描述: 移除脚本语言方案，改为纯 C++/蓝图 GOAP 架构；更新 Thought 文档和 SVG 实现边界。

- 补充 GOAP A* 建模说明: 解释为什么搜索节点是 WorldState 而不是 Action，以及 State 在排序、去重、终止判断、启发函数和回溯中的作用。
- 新增 WorldState 设计讨论: 评估 Settings 注册 State Key、Action 下拉选择、运行期 Index/Mask 编译、类型校验和 Registry 生命周期方案。
- 继续 WorldState 实现讨论: 确认 KeyDef.Type 为权威类型、InState/OutState 改为 Preconditions/Effects，并补充 Mask、Registry、FGuid、DefaultValue、运行层分离等判断理由。
