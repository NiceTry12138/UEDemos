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
