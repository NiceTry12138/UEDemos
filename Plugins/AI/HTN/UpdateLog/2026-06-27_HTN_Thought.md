# 修改内容：HTN 原理学习、算法设计、代码框架、计算过程与实现方案记录

日期：2026-06-27

本次未修改功能代码，只在 `Plugins/AI/HTN` 插件内新增设计与思考记录。

新增文件：

- `Thought/2026-06-27_HTN_Overview.md`
- `Thought/HTN_Architecture.svg`
- `Thought/HTN_Planning_Process.svg`

记录内容：

- 解释 HTN 与行为树的关系和差异。
- 梳理 HTN 核心概念：World State、Task、Method、Operator、Plan、Planner、Executor。
- 给出递归规划算法原理和伪代码。
- 用 `HandleEnemy` 示例演示计算过程。
- 给出 UE 插件内纯 C++ / 蓝图 / DataAsset 的代码框架建议。
- 给出分阶段实现方案、调试方案和工程风险。


修订内容：

- 移除旧版外部配置方案描述。
- 将 Domain 配置改为 UDataAsset + 蓝图派生类。
- 将执行扩展改为 UHTNOperator C++ 基类 + BlueprintNativeEvent。
- 将工程风险中的外部回调成本改为蓝图回调成本风险。

再次修订内容：

- 强化 HTN 定义：从工程框架改为分层规划算法 / 形式化方法。
- 补充 HTN 与 GOAP 对比。
- 补充总序 HTN 与偏序 HTN 说明。
- 修正递归伪代码中 Tasks 分支污染问题。
- 补充 Method 语义边界、Compound Task 子类型、重规划过期策略、Partial Replanning 接口预留。
- 调整 Planner 状态表述：世界状态优先值类型快照，外部对象使用 ID / 句柄查询。
- 补充 Method 固定优先级、动态评分、A* / Best-First 策略差异。
- 增加 SHOP2、PyHOP、Fluid HTN、Killzone / Horizon GDC 参考资料。

算法原理扩写修订：

- 重排文档结构：先算法原理、计算过程、计算示例，再提供结构体和伪代码。
- 扩写 HTN PlanningNode 计算模型：SimWorldState、TaskStack、PlanSoFar、Trace。
- 补充输入、中间状态、输出内容、成功/失败结果定义。
- 补充逐步计算流程、失败分支回溯规则、计算成本和搜索限制。
- 扩写 HandleEnemy 示例为逐步计算表，并明确最终 FHTNPlan 输出。
- 新增核心概念结构体实现草案和 Planner 伪代码。

学术归类与工程模型修订：

- 修正 HTN 归类：HTN 属于 Hierarchical Planning，和 Classical Planning 并列讨论；GOAP 更接近 Classical Planning 工程应用。
- 将 PendingTasks 术语修正为 TaskStack，说明总序 HTN 的栈 / 前向链表语义。
- 修正 Compound Sequence 语义：Sequence 是单一路径顺序必成，不是多个 Method 依次展开。
- 扩展示例为 EnemyHP 多次射击计划，避免单发直接击杀。
- 将条件和效果操作从 FName 改为 enum 草案，补共享 NodeBudget / Deadline。
- 显式加入 UHTNSensor / Context、Utility 排序、Plan 持久化、PlanEffects 与实际效果漂移风险。

SVG 图片内嵌修订：

- 将 `HTN_Planning_Process.svg` 内嵌到第 6 节计算过程示例后。
- 将 `HTN_Architecture.svg` 内嵌到第 8 节 UE 插件代码框架建议开头。
- 将末尾配套 SVG 改为图表索引，避免只在文档末尾出现图片。
