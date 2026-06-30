# HTN 算法原理、代码框架、计算过程与 UE 插件实现方案

本文记录 HTN（Hierarchical Task Network，分层任务网络）学习与实现设计。范围限定在 `Plugins/AI/HTN` 插件内，当前不修改功能代码。

## 1. HTN 是什么

HTN 是一类分层规划算法 / 形式化方法，与 STRIPS、GOAP、PDDL 同属 Classical Planning 大家族。它和 STRIPS / GOAP 这类扁平规划相对，强调“任务分解”，不是“目标状态搜索”。它不直接问“下一帧该做什么”，而是把一个高层任务逐层拆成更小任务，直到得到可执行的原子动作序列。

核心思想：

- 智能体有一个高层任务，例如 `赢得战斗`、`完成巡逻`、`收集资源并建造建筑`。
- 高层任务是复合任务，不能直接执行。
- 复合任务通过 Method（方法）拆解成子任务列表。
- 子任务继续拆解，直到变成 Primitive Task（原子任务）。
- 原子任务绑定 Operator（操作器），有前置条件、执行逻辑、效果。
- Planner 用当前世界状态搜索出一条合法 Plan。
- Executor 按 Plan 执行动作；世界变化后可局部重规划或全量重规划。

HTN 类似行为树，但它更偏“先生成计划，再执行计划”。行为树更偏“每 tick 从树根重新决策”。HTN 适合多步骤、有依赖、有资源状态、有中长期目标的 AI。

## 2. 核心概念

| 概念 | 说明 | UE 插件建议类型 |
| --- | --- | --- |
| World State | AI 认知中的世界状态，如 HP、目标距离、弹药、是否有掩体 | `FHTNWorldState` |
| Task | 任务基类，分复合任务和原子任务 | `UHTNTask` |
| Compound Task | 复合任务，只能被 Method 拆解 | `UHTNCompoundTask` |
| Primitive Task | 原子任务，可绑定 Operator 执行 | `UHTNPrimitiveTask` |
| Method | 拆解规则，包含前置条件和子任务列表 | `UHTNMethod` |
| Operator | 原子动作执行器，包含执行逻辑和效果 | `UHTNOperator` |
| Preconditions | 前置条件，判断 Method 或 Operator 是否可用 | `FHTNCondition` |
| Effects | 计划模拟效果，更新临时世界状态 | `FHTNEffect` |
| Plan | 原子动作序列，Planner 输出 | `FHTNPlan` |
| Planner | 递归拆解和搜索模块 | `FHTNPlanner` |
| Executor | 按计划执行动作，处理失败和重规划 | `UHTNComponent` |
### 2.1 Method 语义边界

Method 是“分解策略是否适用”的判断，不是“子任务一定能成功”的保证。

正确边界：

- Method Preconditions：判断当前是否应该采用这种分解方式。例如 `RangedAttack` 要求 `EnemyVisible && WeaponReady`。
- SubTask Preconditions：判断每个子任务自己能否执行。例如 `ReloadWeapon` 自己判断是否能换弹。
- Operator Preconditions：判断原子动作在计划期和执行期是否可用。

反模式：把所有子任务条件都堆到 Method 上。结果是 Method 过重、复用差、回溯信息不清楚，后续改一个子任务会影响整条分解链。

### 2.2 Compound Task 子类型

第一版建议把 `Compound Task` 明确定义为 Selector：按 Method 固定顺序尝试，选第一个可完成计划的 Method。这与当前伪代码一致，最容易调试。

可预留二期扩展：

- Selector：按顺序选择第一个可用 Method。当前默认。
- Sequence：多个 Method 依次展开，更接近行为树 Sequence。
- Random Selector：按权重随机选择 Method，适合非关键行为多样化。

Fluid HTN 常见做法是把 Compound Task 明确拆成不同类型，避免 Selector / Sequence 语义混在一个类里。

## 3. HTN 与行为树差异

| 维度 | HTN | 行为树 |
| --- | --- | --- |
| 决策方式 | 先搜索计划，再执行 | 每 tick 从根节点运行 |
| 表达重点 | 任务分解、前置条件、状态效果 | 控制流、条件、动作 |
| 中长期行为 | 强，天然支持多步计划 | 需要手写状态机或复杂树结构 |
| 动态响应 | 依赖重规划策略 | Tick 级响应更直接 |
| 调试对象 | 当前计划、拆解路径、状态变化 | 当前运行节点路径 |
| 适合场景 | 战术 AI、生产链、任务链、资源调度 | 即时反应、动作组合、简单状态逻辑 |

HTN 和行为树可以共存：行为树负责顶层感知和模式切换，HTN 负责某个模式内的复杂计划生成。
## 3.1 HTN 与 GOAP 差异

游戏 AI 圈里，HTN 最重要的对照对象不是行为树，而是 GOAP（如《F.E.A.R.》和《巫师 3》部分系统）。两者都会生成计划，但输入模型和搜索方向不同。

| 维度 | HTN | GOAP |
| --- | --- | --- |
| 输入 | 根任务 + 分解方法 | 目标状态 |
| 搜索方向 | 自顶向下分解任务 | 自底向上拼接 Action，常用 A* |
| 设计成本 | 需要写 Method，领域知识强 | 需要写 Action 前置条件和效果 |
| 行为形态 | 受 Method 控制，更像人写的剧本 | 算法自由组合，可能出现意外解 |
| 可预测性 | 高，调试路径接近设计意图 | 中，取决于代价函数和启发式 |
| 扩展成本 | 新策略通常加 Method | 新能力通常加 Action |
| 典型适用 | 战术套路、任务链、强导演行为 | 开放目标、多路径工具组合 |

简单判断：

- 行为要像设计师写好的战术剧本，优先 HTN。
- 行为只关心达成目标，允许系统自由拼 Action，优先 GOAP。
- UE 插件第一版做 HTN，比做 GOAP 更适合战斗 AI、任务链和可解释调试。

## 4. 算法原理

HTN 规划输入：

- 初始世界状态 `S0`
- 根任务 `RootTask`
- 任务库 `Tasks`
- 方法库 `Methods`
- 操作器库 `Operators`

输出：

- 一条原子动作序列 `Plan = [Op1, Op2, ...]`
- 或失败原因，例如无可用 Method、原子任务前置条件失败、搜索深度超限

总序 HTN（Total-Order HTN）：子任务必须按声明顺序执行，规划器每次处理队头任务。偏序 HTN（Partial-Order HTN / POCL）：子任务之间只声明依赖关系，调度时可重排或并行，实现复杂度显著上升。SHOP / SHOP2 是经典总序 HTN 实现，PyHOP 是 Python 简化版。第一版建议只做总序 HTN。

基本递归算法：

```cpp
bool PlanTasks(WorldState State, TaskList Tasks, Plan OutPlan)
{
    if (Tasks.IsEmpty())
    {
        return true;
    }

    Task Current = Tasks.PopFront();
    TaskList Remaining = Tasks; // 已经移除 Current，后续分支都必须用同一份剩余任务

    if (Current.IsPrimitive())
    {
        Operator Op = Current.GetOperator();
        if (!Op.CheckPreconditions(State))
        {
            return false;
        }

        OutPlan.Add(Op);
        Op.ApplyEffects(State);
        return PlanTasks(State, Remaining, OutPlan);
    }

    for (Method M : Current.GetMethods())
    {
        if (!M.CheckPreconditions(State))
        {
            continue;
        }

        WorldState BranchState = State.Clone();
        Plan BranchPlan = OutPlan.Clone();
        TaskList Expanded = M.SubTasks + Remaining;

        if (PlanTasks(BranchState, Expanded, BranchPlan))
        {
            OutPlan = BranchPlan;
            return true;
        }
    }

    return false;
}
```

关键点：

- Planner 不应该直接修改真实世界状态，只修改模拟状态。`FHTNWorldState` 应该是 POD-like 的快照值，例如 HP、距离、bool flag、资源数量。
- Method 顺序会影响计划结果。第一版用固定优先级，后续可扩展动态评分、冷却、随机权重。
- Method Preconditions 只判断“这种分解策略是否适用”，不要替子任务判断全部成功条件。
- Operator Effects 是“计划期预测效果”，真实执行仍可能失败。
- 执行失败后不要强行继续计划，应触发重规划或 fallback。
- 需要限制最大深度、最大节点数、最大耗时，避免死递归。

## 5. 计算过程示例

示例目标：`处理敌人`

初始世界状态：

```text
HasEnemy = true
EnemyVisible = true
Ammo = 0
HasCover = true
DistanceToEnemy = 1200
WeaponReady = true
```

根任务：

```text
HandleEnemy
```

任务库：

```text
HandleEnemy               // 复合任务
  Method A: RangedAttack
    条件: EnemyVisible && WeaponReady
    子任务: EnsureAmmo -> MoveToCover -> ShootEnemy

  Method B: MeleeAttack
    条件: EnemyVisible && DistanceToEnemy < 200
    子任务: MoveToEnemy -> MeleeHit

EnsureAmmo                // 复合任务
  Method A: AlreadyHaveAmmo
    条件: Ammo > 0
    子任务: None

  Method B: Reload
    条件: Ammo == 0
    子任务: ReloadWeapon

MoveToCover               // 原子任务
ShootEnemy                // 原子任务
ReloadWeapon              // 原子任务
```

规划展开：

1. `HandleEnemy` 是复合任务。
2. 检查 `RangedAttack`，条件满足，展开为 `EnsureAmmo -> MoveToCover -> ShootEnemy`。
3. `EnsureAmmo` 是复合任务。
4. `AlreadyHaveAmmo` 条件失败，因为 `Ammo == 0`。
5. `Reload` 条件成功，展开为 `ReloadWeapon`。
6. `ReloadWeapon` 是原子任务，检查通过，模拟效果：`Ammo = 30`。
7. `MoveToCover` 是原子任务，检查通过，模拟效果：`InCover = true`。
8. `ShootEnemy` 是原子任务，检查通过，模拟效果：`EnemyAlive = false`、`Ammo -= 1`。
9. 输出计划：`ReloadWeapon -> MoveToCover -> ShootEnemy`。

失败与回溯：

- 如果 `ReloadWeapon` 前置条件失败，Planner 回到 `EnsureAmmo` 选择其他 Method。
- 如果 `EnsureAmmo` 无可用 Method，Planner 回到 `HandleEnemy`，尝试 `MeleeAttack`。
- 如果所有分支失败，规划失败，Executor 进入 fallback，例如撤退、待机、请求新目标。

## 6. 代码框架建议

插件目录建议：

```text
Plugins/AI/HTN/
  HTN.uplugin
  Source/
    HTN/
      HTN.Build.cs
      Public/
        HTNTypes.h
        HTNWorldState.h
        HTNCondition.h
        HTNEffect.h
        HTNTask.h
        HTNMethod.h
        HTNOperator.h
        HTNPlan.h
        HTNPlanner.h
        HTNComponent.h
      Private/
        HTNWorldState.cpp
        HTNCondition.cpp
        HTNEffect.cpp
        HTNTask.cpp
        HTNMethod.cpp
        HTNOperator.cpp
        HTNPlan.cpp
        HTNPlanner.cpp
        HTNComponent.cpp
  Content/
    AI/
      HTN/
        DA_HTNDomain.uasset
  Thought/
  UpdateLog/
```

建议 C++ 负责核心规划、运行时执行和 UE 生命周期；任务库、方法库、操作器配置通过 `UDataAsset` / 蓝图派生类暴露给设计侧。第一版尽量让计划期条件和效果数据化，避免在 Planner 高频搜索中大量调用蓝图逻辑。

### 6.1 C++ 类型草案

```cpp
UENUM(BlueprintType)
enum class EHTNTaskType : uint8
{
    Compound,
    Primitive
};

USTRUCT(BlueprintType)
struct FHTNWorldState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, int32> IntValues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, bool> BoolValues;
};

USTRUCT(BlueprintType)
struct FHTNCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Op;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Value;
};

UCLASS(BlueprintType)
class UHTNMethod : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName MethodId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FHTNCondition> Preconditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> SubTasks;
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class UHTNComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    bool BuildPlan(FName RootTask);
    void TickPlan(float DeltaSeconds);
    void AbortPlan(FName Reason);
};
```

### 6.2 DataAsset / 蓝图配置草案

```cpp
UCLASS(BlueprintType)
class UHTNDomainDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TMap<FName, TObjectPtr<UHTNTask>> Tasks;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TMap<FName, TObjectPtr<UHTNOperator>> Operators;
};

UCLASS(BlueprintType, Blueprintable, Abstract)
class UHTNOperator : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName OperatorId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FHTNCondition> Preconditions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FHTNEffect> PlanEffects;

    UFUNCTION(BlueprintNativeEvent)
    EHTNOperatorResult StartOperator(AActor* OwnerActor);

    UFUNCTION(BlueprintNativeEvent)
    EHTNOperatorResult TickOperator(AActor* OwnerActor, float DeltaSeconds);

    UFUNCTION(BlueprintNativeEvent)
    void AbortOperator(AActor* OwnerActor);
};
```

蓝图侧使用方式：

```text
DA_HTNDomain
  Tasks
    HandleEnemy
      Type = Compound
      Methods = RangedAttack, MeleeAttack

    ReloadWeapon
      Type = Primitive
      Operator = BP_HTNOp_ReloadWeapon

  Methods
    RangedAttack
      Preconditions = EnemyVisible == true, WeaponReady == true
      SubTasks = EnsureAmmo, MoveToCover, ShootEnemy

  Operators
    BP_HTNOp_ReloadWeapon
      Preconditions = CanReload == true
      PlanEffects = Ammo set 30
      Runtime Logic = 蓝图或 C++ 重写 Start/Tick/Abort
```

## 7. UE 实现方案

### 阶段 1：最小可用 Planner

- 实现 `FHTNWorldState`，支持 bool/int/float/name 基础读写。
- 实现 `FHTNCondition`，支持 `==`、`!=`、`>`、`>=`、`<`、`<=`。
- 实现 `FHTNEffect`，支持 `set`、`add`、`remove`。
- 实现 `UHTNTask`、`UHTNMethod`、`UHTNOperator` 数据对象。
- 实现深度优先总序 Planner。
- 输出 `FHTNPlan`，只包含 Operator Id 和调试路径。

### 阶段 2：执行与重规划

- `UHTNComponent` 持有当前计划、当前步骤、执行状态。
- Operator 有 `Start`、`Tick`、`Finish`、`Abort`。
- 任一 Operator 执行失败，触发 `OnPlanFailed`。
- 默认重规划策略：关键状态变化 + 执行失败兜底。
- 支持重规划条件：
  - 当前 Operator 失败。
  - 关键世界状态变化：Domain 声明 `WatchedKeys`，例如 `EnemyVisible`、`Ammo`、`TargetId`，任一变化触发重规划。
  - Sensor Dirty Flag：感知模块标记“世界变了”，下一帧或下个安全点重规划。
  - Plan TTL：计划带时限，超时强制重规划。第一版可选，不做默认主策略。
  - 外部强制切换 RootTask。
- 乐观执行 + 失败回退可以作为低成本兜底，但不应替代关键状态监听。

### 阶段 3：DataAsset Domain 与蓝图扩展

- C++ 提供 `UHTNDomainDataAsset`，集中保存 tasks/methods/operators。
- 设计侧在 Content 下创建 `DA_HTNDomain`，通过编辑器配置任务拆解。
- 原子动作使用 `UHTNOperator` C++ 基类，可派生蓝图类实现运行时 Start/Tick/Abort。
- Planner 只读取 DataAsset 里的数据化条件和效果；复杂运行逻辑留到 Executor 阶段。
- 必要时提供 `UHTNConditionProvider` 蓝图扩展点，但默认不在高频搜索路径依赖蓝图回调。

### 阶段 4：调试与可视化

- 输出每次规划展开 trace。
- 记录每个 Method 为什么通过或失败。
- 显示当前计划步骤、当前世界状态、失败原因。
- 编辑器中提供 Domain 检查工具：
  - 未引用 Task
  - 不存在 SubTask
  - 循环拆解
  - 永远不可达 Method
  - 无 Operator 的 Primitive Task

## 8. 关键工程风险

- 递归爆炸：必须有最大深度、最大展开节点、最大耗时。
- Method 顺序过强：需要优先级或评分，否则行为容易被配置顺序锁死。
- Effect 与真实执行不一致：计划认为成功，实际世界失败，必须重规划。
- 蓝图回调过重：规划可能频繁运行，计划期条件建议数据化或 C++ 化，蓝图主要负责低频运行时动作。
- Debug 缺失：HTN 失败时很难看懂，trace 是必需功能。
- UE 对象生命周期：Planner 的搜索状态不应耦合 UObject 生命周期。`FHTNWorldState` 优先保存值类型快照；外部对象用 `FName` / `FGuid` / GameplayTag / 轻量句柄表示，再通过上下文查询真实 Actor。不要把 `TWeakObjectPtr<AActor>` 当作核心计划状态，否则 GC 或对象销毁会让 trace 难以复现。
- 局部重规划接口：第一版可以全量重规划，但 `FHTNPlan` 应保留当前步骤、剩余任务栈、RootTask、WorldState Snapshot，避免未来做 Partial Replanning 时推翻数据结构。

## 9. 推荐默认策略

- 第一版使用总序 HTN，不做偏序规划。
- 第一版使用 DFS + 固定 Method 优先级，不做 A* / Best-First。
- Method 选择策略分三档：
  - 固定优先级：Method 数组顺序就是优先级。最简单，行为可预测，但较僵硬。
  - 动态评分（Utility）：每次规划按世界状态算分。行为更灵活，但要设计评分函数。
  - A* / Best-First：把 Method 选择也纳入启发式搜索。复杂，第一版不做。
- 第一版仍预留评分接口，例如 `GetMethodScore(WorldState)` 默认返回常量或数组序号，二期可无痛升级。
- 第一版 Operator Effects 只做轻量预测，不模拟完整物理或动画。
- 第一版计划长度限制 32，递归深度限制 16，展开节点限制 512。
- 第一版所有正式代码放 `Plugins/AI/HTN/Source`，不放 `Empty_54/Source`。

## 10. 参考实现与资料

- SHOP2：学术界经典 HTN 实现，LISP，适合读论文和理解总序 HTN 形式化定义。
- PyHOP / Pyhop：Python 简化实现，代码量小，适合快速读懂递归分解核心。
- Fluid HTN：C# 开源实现，游戏工程语境强。Domain Builder DSL、Compound Task 分类、Context / Effect / Condition 结构都适合参考到 UE 插件。
- Killzone / Horizon Zero Dawn GDC 演讲：适合参考大型游戏项目如何把 HTN 用在战术 AI、调试工具和设计师工作流里。

## 11. 配套 SVG

- [HTN 架构图](./HTN_Architecture.svg)
- [HTN 规划计算过程图](./HTN_Planning_Process.svg)

