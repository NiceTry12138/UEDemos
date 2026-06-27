# HTN 算法原理、代码框架、计算过程与 UE 插件实现方案

本文记录 HTN（Hierarchical Task Network，分层任务网络）学习与实现设计。范围限定在 `Plugins/AI/HTN` 插件内，当前不修改功能代码。

## 1. HTN 是什么

HTN 是一种 AI 规划框架。它不直接问“下一帧该做什么”，而是把一个高层任务逐层拆成更小任务，直到得到可执行的原子动作序列。

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

基本递归算法：

```cpp
bool PlanTasks(WorldState State, TaskList Tasks, Plan OutPlan)
{
    if (Tasks.IsEmpty())
    {
        return true;
    }

    Task Current = Tasks.PopFront();

    if (Current.IsPrimitive())
    {
        Operator Op = Current.GetOperator();
        if (!Op.CheckPreconditions(State))
        {
            return false;
        }

        OutPlan.Add(Op);
        Op.ApplyEffects(State);
        return PlanTasks(State, Tasks, OutPlan);
    }

    for (Method M : Current.GetMethods())
    {
        if (!M.CheckPreconditions(State))
        {
            continue;
        }

        WorldState BranchState = State.Clone();
        Plan BranchPlan = OutPlan.Clone();
        TaskList Expanded = M.SubTasks + Tasks;

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

- Planner 不应该直接修改真实世界状态，只修改模拟状态。
- Method 顺序会影响计划结果。可以用优先级、评分、冷却、随机权重控制。
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
  Scripts/
    HTN/
      tasks.lua
      methods.lua
      operators.lua
  Thought/
  UpdateLog/
```

建议 C++ 负责核心规划和 UE 生命周期，Lua 负责配置任务、条件、方法和操作器 glue 逻辑。

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

### 6.2 Lua 配置草案

```lua
return {
  tasks = {
    HandleEnemy = {
      type = "compound",
      methods = { "RangedAttack", "MeleeAttack" }
    },
    ReloadWeapon = {
      type = "primitive",
      operator = "OpReloadWeapon"
    },
    MoveToCover = {
      type = "primitive",
      operator = "OpMoveToCover"
    },
    ShootEnemy = {
      type = "primitive",
      operator = "OpShootEnemy"
    }
  },

  methods = {
    RangedAttack = {
      preconditions = {
        { key = "EnemyVisible", op = "==", value = true },
        { key = "WeaponReady", op = "==", value = true }
      },
      subtasks = { "EnsureAmmo", "MoveToCover", "ShootEnemy" }
    }
  },

  operators = {
    OpReloadWeapon = {
      preconditions = {
        { key = "CanReload", op = "==", value = true }
      },
      effects = {
        { key = "Ammo", op = "set", value = 30 }
      }
    }
  }
}
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
- 支持重规划条件：
  - 当前 Operator 失败。
  - 关键世界状态变化。
  - 计划过期。
  - 外部强制切换 RootTask。

### 阶段 3：Lua Domain 接入

- C++ 提供 Domain Loader。
- Lua 返回 tasks/methods/operators 表。
- C++ 解析成运行时对象。
- Lua 可提供自定义 condition/operator callback，但核心搜索仍在 C++。

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
- Lua 回调过重：规划可能频繁运行，Lua 条件要缓存或限流。
- Debug 缺失：HTN 失败时很难看懂，trace 是必需功能。
- UE 对象生命周期：Planner 内部搜索不应持有易失 Actor 强引用，推荐用弱引用或上下文查询。

## 9. 推荐默认策略

- 第一版使用总序 HTN，不做偏序规划。
- 第一版使用 DFS + Method 优先级，不做 A*。
- 第一版 Operator Effects 只做轻量预测，不模拟完整物理或动画。
- 第一版计划长度限制 32，递归深度限制 16，展开节点限制 512。
- 第一版所有正式代码放 `Plugins/AI/HTN/Source`，不放 `Empty_54/Source`。

## 10. 配套 SVG

- [HTN 架构图](./HTN_Architecture.svg)
- [HTN 规划计算过程图](./HTN_Planning_Process.svg)

