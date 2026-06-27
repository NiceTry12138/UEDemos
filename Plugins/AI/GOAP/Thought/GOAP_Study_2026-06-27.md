# GOAP 学习与实现方案

修改内容: GOAP 算法原理、代码框架、计算过程、UE 插件实现方案、SVG 图解。

配图: [GOAP_Algorithm_Flow.svg](./GOAP_Algorithm_Flow.svg)

## 1. GOAP 是什么

GOAP, Goal Oriented Action Planning, 是一种基于目标和动作规划的 AI 驱动方式。

它不像行为树那样提前把决策流程写成固定树结构，而是把 AI 的能力拆成很多 Action。每个 Action 声明:

- Preconditions: 执行动作前必须满足的世界状态。
- Effects: 动作执行后会改变哪些世界状态。
- Cost: 动作代价，用来选择更优方案。

运行时，AI 先知道当前 World State，再选择一个 Goal，然后用搜索算法从当前状态推导出一串 Action，使最终状态满足 Goal。

一句话: 行为树是手写决策流程，GOAP 是运行时按目标自动拼动作链。

## 2. GOAP 与行为树对比

| 维度 | 行为树 | GOAP |
| --- | --- | --- |
| 决策方式 | 人工组织节点执行顺序 | 自动搜索动作序列 |
| 结构 | 树状、显式流程 | 状态、目标、动作集合 |
| 适合场景 | 流程清晰、可控性强的 AI | 目标多、动作组合多、需要涌现行为的 AI |
| 调试重点 | 哪个节点失败、哪个分支运行 | 当前状态、目标、规划路径、动作代价 |
| 缺点 | 复杂 AI 容易树爆炸 | 规划成本更高，状态设计要求更严 |

两者不是互斥。常见做法是:

- 行为树负责高层节奏，比如巡逻、战斗、逃跑状态切换。
- GOAP 负责某个状态内的动作规划，比如战斗中如何找掩体、换弹、靠近、攻击。

## 3. 核心数据模型

### 3.1 World State

World State 是一组键值事实。

示例:

```text
HasWeapon = true
EnemyVisible = true
Ammo = 0
InCover = false
EnemyAlive = true
DistanceToEnemy = Far
```

实现上建议不要一开始做成任意 Variant 大杂烩。UE 插件里可分两层:

- C++ 内部用紧凑结构做规划，例如 FName + bool/int/enum。
- Lua 层暴露可读写接口，用于条件判断和效果计算。

### 3.2 Goal

Goal 描述 AI 想达成的最终状态。

示例:

```text
Goal: KillEnemy
DesiredState:
  EnemyAlive = false
Priority = 100
```

Goal 通常需要 Priority。多个目标同时可选时，先选优先级最高或效用最高的目标，再规划。

### 3.3 Action

Action 是 GOAP 的最小执行单元。

示例:

```text
Action: Reload
Preconditions:
  HasWeapon = true
  Ammo = 0
Effects:
  Ammo > 0
Cost: 2
Executor: Lua function or C++ task
```

一个 Action 最少需要:

- Id: 唯一名称。
- Preconditions: 规划前置条件。
- Effects: 规划结果。
- Cost: 规划代价。
- CanRun: 运行时二次校验。
- Start/Tick/Abort/Finish: 实际执行接口。

规划时只看 Preconditions、Effects、Cost。执行时还要校验 CanRun，因为世界会变。

## 4. 算法原理

GOAP 的本质是图搜索。

- 节点: 一个 World State。
- 边: 一个 Action。
- 起点: 当前 World State。
- 终点: 满足 Goal 的 World State。
- 边权: Action Cost。

常用算法:

- Dijkstra: 能找到最低 Cost 路径，但没有启发函数，搜索可能更慢。
- A*: 加上 Heuristic 后更快，适合动作数量较多的项目。
- Backward Planning: 从 Goal 反推需要哪些前置条件，再找能满足这些条件的 Action。

推荐实现:

- 第一版: Forward A*，更直观，方便调试。
- 优化版: Backward A* 或双向规划，减少搜索空间。

## 5. Forward A* 计算过程

输入:

- CurrentState
- GoalState
- ActionSet

过程:

1. 创建 OpenSet，把当前状态作为起点放入。
2. 从 OpenSet 取 FScore 最低的节点。
3. 如果节点状态满足 GoalState，回溯 Parent Action，得到 Plan。
4. 遍历所有 Action。
5. 如果 Action.Preconditions 被当前节点状态满足，则应用 Action.Effects，生成 NextState。
6. 计算代价:

```text
GScore = Parent.GScore + Action.Cost
HScore = Heuristic(NextState, GoalState)
FScore = GScore + HScore
```

7. 如果 NextState 没访问过，或新 GScore 更低，则更新 Parent 并放入 OpenSet。
8. 循环直到找到 Plan 或 OpenSet 为空。

伪代码:

```cpp
Plan BuildPlan(State Start, Goal Goal, Array<Action> Actions)
{
    OpenSet.Push(Node(Start, G = 0, H = Estimate(Start, Goal)));

    while (!OpenSet.IsEmpty())
    {
        Node Current = OpenSet.PopLowestF();

        if (Goal.IsSatisfiedBy(Current.State))
        {
            return ReconstructPlan(Current);
        }

        for (Action Action : Actions)
        {
            if (!Action.Preconditions.Match(Current.State))
            {
                continue;
            }

            State NextState = Current.State.Apply(Action.Effects);
            float NextG = Current.G + Action.Cost;

            if (Visited.HasBetterOrEqual(NextState, NextG))
            {
                continue;
            }

            OpenSet.Push(Node(
                State = NextState,
                Parent = Current,
                ParentAction = Action,
                G = NextG,
                H = Estimate(NextState, Goal)));
        }
    }

    return EmptyPlan;
}
```

## 6. 示例计算

当前状态:

```text
HasWeapon = true
Ammo = 0
EnemyVisible = true
InCover = false
EnemyAlive = true
```

目标:

```text
EnemyAlive = false
```

动作集合:

| Action | Preconditions | Effects | Cost |
| --- | --- | --- | --- |
| Reload | HasWeapon=true, Ammo=0 | Ammo=6 | 2 |
| Shoot | HasWeapon=true, Ammo>0, EnemyVisible=true | EnemyAlive=false, Ammo-=1 | 1 |
| FindCover | InCover=false | InCover=true | 3 |
| Aim | EnemyVisible=true | Aiming=true | 1 |

规划结果可能是:

```text
Reload -> Shoot
TotalCost = 3
```

如果 Shoot 增加前置条件 Aiming=true，则结果变为:

```text
Reload -> Aim -> Shoot
TotalCost = 4
```

如果敌人不可见，且有 SearchEnemy:

```text
SearchEnemy -> Reload -> Aim -> Shoot
```

这就是 GOAP 的核心价值: 不改一棵大行为树，只要添加或修改 Action 的条件和效果，AI 就能重新组合动作。

## 7. UE 插件代码框架建议

目录建议:

```text
Plugins/AI/GOAP
  Source/GOAPRuntime
    Public
      GOAPTypes.h
      GOAPWorldState.h
      GOAPAction.h
      GOAPGoal.h
      GOAPPlanner.h
      GOAPAgentComponent.h
    Private
      GOAPWorldState.cpp
      GOAPAction.cpp
      GOAPGoal.cpp
      GOAPPlanner.cpp
      GOAPAgentComponent.cpp
  Content
  Thought
  UpdateLog
```

### 7.1 C++ 职责

C++ 做核心、稳定、性能敏感部分:

- WorldState 存储与比较。
- Preconditions 匹配。
- Effects 应用。
- A* 或 Dijkstra 搜索。
- Plan 队列管理。
- AgentComponent 生命周期。
- 调试信息输出。

关键类:

```cpp
struct FGOAPStateValue
{
    EGOAPValueType Type;
    bool BoolValue;
    int32 IntValue;
    FName NameValue;
};

struct FGOAPWorldState
{
    TMap<FName, FGOAPStateValue> Facts;

    bool Matches(const FGOAPWorldState& Conditions) const;
    FGOAPWorldState Apply(const FGOAPWorldState& Effects) const;
    uint32 GetStableHash() const;
};

class UGOAPAction : public UObject
{
public:
    FName ActionId;
    float Cost;
    FGOAPWorldState Preconditions;
    FGOAPWorldState Effects;

    virtual bool CanRun(AActor* Owner) const;
    virtual void Start(AActor* Owner);
    virtual EGOAPActionStatus Tick(AActor* Owner, float DeltaSeconds);
    virtual void Abort(AActor* Owner);
};

class FGOAPPlanner
{
public:
    bool BuildPlan(
        const FGOAPWorldState& CurrentState,
        const FGOAPGoal& Goal,
        const TArray<UGOAPAction*>& Actions,
        FGOAPPlan& OutPlan);
};

class UGOAPAgentComponent : public UActorComponent
{
public:
    void SetGoal(FName GoalId);
    void RequestReplan();
    void TickPlan(float DeltaSeconds);
};
```

### 7.2 Lua 职责

Lua 做可配置、快速迭代部分:

- 定义具体动作。
- 动态条件检查。
- 动作执行逻辑。
- 读取 UE Actor 状态。
- 写入黑板或 GOAP WorldState。

Lua Action 示例:

```lua
return {
    id = "Reload",
    cost = 2,
    preconditions = {
        HasWeapon = true,
        AmmoEmpty = true,
    },
    effects = {
        AmmoReady = true,
    },

    can_run = function(agent)
        return agent:HasWeapon() and agent:GetAmmo() == 0
    end,

    start = function(agent)
        agent:PlayMontage("Reload")
    end,

    tick = function(agent, dt)
        if agent:IsMontageFinished("Reload") then
            agent:SetAmmo(6)
            return "Success"
        end
        return "Running"
    end,

    abort = function(agent)
        agent:StopMontage("Reload")
    end,
}
```

### 7.3 C++ 与 Lua 边界

建议边界:

- C++ 负责规划器和状态数据结构。
- Lua 负责具体 Action 的条件、效果声明和执行。
- 规划时尽量少调用 Lua，避免高频搜索开销。
- 动态世界信息先同步成 WorldState，再交给 C++ Planner。

不建议:

- 每次扩展节点都调用 Lua 判断复杂条件。
- WorldState 使用任意字符串表达式。
- 把执行中副作用写进 Planner。

## 8. Agent Tick 流程

每个 GOAPAgentComponent Tick:

1. 收集感知、属性、黑板数据，刷新 CurrentWorldState。
2. 选择当前最高优先级 Goal。
3. 如果没有 Plan，或 Goal 改变，或当前 Action 失效，则 RequestReplan。
4. Planner 生成 Action 队列。
5. 执行队首 Action。
6. Action 成功则出队。
7. Action 失败则清空 Plan 并重新规划。
8. Goal 达成则选择下一个 Goal。

运行时必须区分两个阶段:

- Planning: 在抽象状态空间里推演。
- Execution: 在真实世界里执行动作。

真实世界会变化，所以执行阶段失败是正常情况，GOAP 必须支持 Replan。

## 9. 实现方案分期

### Phase 1: 最小可用版本

目标: 能规划并执行固定动作链。

- 实现 bool/int/FName 三种状态值。
- 实现 Preconditions 精确匹配。
- 实现 Effects 覆盖写入。
- 实现 Dijkstra 或 A*。
- 实现 UGOAPAgentComponent。
- 支持 Lua 注册 Action。
- 提供 Debug 输出: CurrentState、Goal、Plan。

### Phase 2: 调试与编辑体验

目标: 让策划和程序能看懂 AI 为什么这样做。

- UE Debug Draw 显示当前 Goal 和 Plan。
- 输出规划失败原因。
- 记录每个 Action 被跳过的 Preconditions。
- 增加规划耗时统计。
- 增加可视化 Plan History。

### Phase 3: 性能与复杂条件

目标: 支持更多 AI 同时运行。

- 缓存可复用 Plan。
- 限制单帧最大搜索节点数。
- 支持异步规划。
- 支持 State Hash 去重。
- 支持 Action Cooldown。
- 支持动态 Cost。

### Phase 4: 与 UE AI 系统融合

目标: 插入 UE 原生 AI 工作流。

- Blackboard 适配器。
- Behavior Tree Task: RunGOAPPlan。
- EQS 查询结果写入 WorldState。
- AIController 适配。
- GameplayTag 作为状态 Key 或 Value。

## 10. 关键风险

### 10.1 状态爆炸

动作越多，状态越细，搜索空间越大。

处理:

- 只把规划需要的信息放入 WorldState。
- 不要把位置、血量浮点值直接作为状态。
- 连续值离散化，例如 DistanceToEnemy = Near/Mid/Far。
- 给搜索加最大节点数和最大耗时。

### 10.2 规划结果和真实执行不一致

规划认为 Reload 后 AmmoReady=true，但真实执行中被打断。

处理:

- 每个 Action 执行前调用 CanRun。
- Action 失败立即 Replan。
- Effects 只表达预期结果，真实结果仍从世界同步。

### 10.3 Action 粒度错误

Action 太大，组合能力弱。Action 太小，搜索成本高。

建议:

- 一个 Action 对应一个稳定、有明确完成条件的游戏行为。
- 移动到某点、换弹、开门、拾取、攻击可以是 Action。
- 每一帧调整朝向这种低层控制不应该是 GOAP Action。

## 11. 推荐落地原则

- GOAP 不替代全部 AI，只负责需要动态组合动作的部分。
- 第一版不要追求通用表达式系统，先把状态和比较规则做简单。
- Action 和 Goal 必须可调试，不然 GOAP 很难定位问题。
- 规划器不要产生真实副作用。
- Lua 可以让 Action 快速迭代，但规划核心建议放 C++。

