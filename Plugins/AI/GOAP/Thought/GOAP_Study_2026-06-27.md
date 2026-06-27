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
- 蓝图层通过 UCLASS / UDataAsset / BlueprintNativeEvent 暴露配置与扩展点，用于编辑 Action、Goal、条件校验和执行逻辑。

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
Executor: C++ class or Blueprint override
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

### 5.1 基本概念

#### OpenSet

OpenSet 是“待探索节点集合”。

在 GOAP 里，一个节点不是地图格子，而是一个 WorldState:

```text
Node
  State: 当前推演出来的世界状态
  Parent: 从哪个旧 Node 来
  ParentAction: 是哪个 Action 把 Parent.State 变成当前 State
  GScore: 从起点走到当前节点已经花了多少 Cost
  HScore: 当前节点距离 Goal 的估算成本
  FScore: GScore + HScore
```

OpenSet 的含义:

```text
这些状态已经被生成出来，但还没展开它们的后续 Action。
```

每轮从 OpenSet 里取 FScore 最低的 Node，因为它看起来最可能用较低总代价到达 Goal。

#### GScore

GScore 是从 StartState 到当前 Node 的真实累计代价。

例如:

```text
Start
  -> Reload(Cost=2)
  -> Aim(Cost=1)

当前 Node.GScore = 3
```

GScore 是已经确定发生的成本，不是猜测。

#### HScore

HScore 是 Heuristic，也就是启发式估算。

它估算当前 State 离 Goal 还有多远。

简单实现:

```text
GoalState 有 3 个条件
当前 State 已满足 1 个
HScore = 2
```

更复杂实现可以按不同 Goal 条件加权。

HScore 只是“估计”，它不一定完全准确。它的作用是让搜索优先朝更像目标的方向走。

#### FScore

FScore 是 A* 排序用的总评分:

```text
FScore = GScore + HScore
```

含义:

```text
从起点走到当前状态已经花的成本
+ 从当前状态到目标状态预计还要花的成本
```

所以:

- GScore 小: 当前路径已经比较便宜。
- HScore 小: 当前状态看起来更接近目标。
- FScore 小: 当前节点整体更值得优先展开。

如果 HScore 永远等于 0，那么 A* 就退化成 Dijkstra。

### 5.2 为什么要记录 Parent 和 ParentAction

搜索过程中，Planner 会生成很多 Node。

例如:

```text
Node0: State = Ammo=0, Aiming=false, EnemyAlive=true

Node1:
  State = Ammo=6, Aiming=false, EnemyAlive=true
  Parent = Node0
  ParentAction = Reload

Node2:
  State = Ammo=6, Aiming=true, EnemyAlive=true
  Parent = Node1
  ParentAction = Aim

Node3:
  State = Ammo=5, Aiming=true, EnemyAlive=false
  Parent = Node2
  ParentAction = Shoot
```

当搜索到 Node3 时，Node3.State 已经满足 Goal:

```text
EnemyAlive = false
```

但此时我们只知道“当前节点满足目标”，还不知道完整动作序列。

因为完整动作序列分散记录在每个 Node 的 ParentAction 里:

```text
Node3.ParentAction = Shoot
Node2.ParentAction = Aim
Node1.ParentAction = Reload
```

所以要从 Goal Node 一路沿 Parent 往回走:

```text
Node3 --Parent--> Node2 --Parent--> Node1 --Parent--> Node0
```

回溯时收集 ParentAction:

```text
Shoot, Aim, Reload
```

这是反的。反转后得到:

```text
Reload -> Aim -> Shoot
```

这就是 Plan。

为什么回溯就能得到 Plan:

- 每个 Node 都是由某个 Parent Node 执行某个 ParentAction 得到的。
- 从 StartNode 到 GoalNode 的 Parent 链，就是一条完整路径。
- 路径上的 ParentAction，就是把 StartState 变成 GoalState 的动作序列。

### 5.3 “NextState 没访问过，或新 GScore 更低”是什么意思

同一个 WorldState 可能由多条路径到达。

例子:

```text
路径 A:
Start -> FindAmmo(Cost=5) -> Reload(Cost=2)
到达 State: Ammo=6
GScore = 7

路径 B:
Start -> PickLoadedWeapon(Cost=3)
到达 State: Ammo=6
GScore = 3
```

两个路径得到的 NextState 都是:

```text
Ammo = 6
```

但路径 B 更便宜。

所以 Planner 需要记录:

```text
某个 State 目前已知的最低 GScore 是多少。
```

当生成 NextState 时:

- 如果这个 State 从没见过，就加入 OpenSet。
- 如果见过，但新 GScore 更低，说明找到一条更便宜路径，要更新它的 Parent 和 ParentAction。
- 如果见过，而且旧 GScore 更低或相等，新路径没有价值，跳过。

“更新 Parent” 的意义:

```text
这个 State 以后如果通向 Goal，回溯 Plan 时应该走更便宜的来源路径。
```

不更新的话，Planner 可能保留旧的高成本路径，最后得到的 Plan 就不是最低代价 Plan。

### 5.4 为什么循环直到找到 Plan 或 OpenSet 为空

主循环条件是:

```text
while OpenSet not empty
```

每轮取一个最值得探索的 Node。

取出后先检查:

```text
if Current.State satisfies GoalState:
    return ReconstructPlan(Current)
```

所以“满足 GoalState 时停止”是对的。更完整说法是:

```text
循环持续运行；
每次弹出一个节点时检查是否满足 Goal；
一旦满足 Goal，就立刻停止并返回 Plan；
如果一直找不到，直到 OpenSet 为空，说明没有可达 Plan。
```

OpenSet 为空代表:

```text
所有从 StartState 出发、能通过 ActionSet 推演出来的状态都探索完了，
仍然没有任何状态满足 GoalState。
```

这时不是继续循环，而是返回 EmptyPlan，表示规划失败。

### 5.5 具体过程

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

### 5.6 与 A* 的关系

这不是普通检索加回溯，而是把 GOAP 问题建模成 A* 搜索。

A* 的标准模型:

```text
地图寻路:
  Node = 地图格子
  Edge = 从一个格子移动到另一个格子
  Cost = 移动成本
  Start = 起点格子
  Goal = 终点格子
```

GOAP 的对应模型:

```text
GOAP 规划:
  Node = 一个 WorldState
  Edge = 一个 Action
  Cost = Action.Cost
  Start = CurrentState
  Goal = GoalState
```

也就是:

```text
地图 A* 找的是: 从起点格子到终点格子的路径
GOAP A* 找的是: 从当前状态到目标状态的动作路径
```

回溯不是 A* 独有，但 A* 常用 Parent 回溯来还原路径。

真正让它成为 A* 的关键是:

```text
每次优先展开 FScore = GScore + HScore 最低的节点。
```

如果没有 FScore 排序，只是随便遍历 Action，那就是普通图搜索。

如果只按 GScore 排序，HScore=0，那是 Dijkstra。

如果只按 HScore 排序，不考虑已经花掉的 GScore，那是贪心最佳优先搜索，可能不是最低成本路径。

Forward A* GOAP 的本质:

```text
用 A* 在“状态图”上搜索最低代价动作序列。
```

### 5.7 为什么节点是 WorldState 而不是 Action

这是 GOAP 学习里最常见的认知盲点。

简短回答: “节点 = State”和“用 Action 驱动搜索”并不冲突。Action 确实负责扩展搜索，但被排序、去重、终止判断、启发估算和回溯的对象是 State。所以算法建模上说:

```text
Node = WorldState
Edge = Action
```

#### 直觉没有错，只是视角不同

一个 Action 的工作方式确实是:

```text
CurrentWorldState --Action--> NextWorldState
```

Planner 也确实会按下面方式扩展:

```text
如果 Action.Preconditions 匹配 CurrentWorldState:
    NextWorldState = Apply(CurrentWorldState, Action.Effects)
    Score = Action.Cost + H(NextWorldState, GoalState)
```

但注意: 当我们说按 `Cost + H(NextWorldState, GoalState)` 决定优先级时，真正被评分的是 `NextWorldState`，不是 Action 本身。

同一个 Action 在不同上下文里会产生不同结果:

```text
StateA --Reload--> Ammo=6, InCover=false
StateB --Reload--> Ammo=6, InCover=true
```

Action 名字一样，但执行后的 WorldState 不一样。A* 需要比较的是“执行后到达哪里”，所以排序载体必须是 State。

换句话说:

```text
实现层面: Planner 遍历 Action 来扩展节点。
算法层面: Planner 在 State 图上搜索路径。
```

#### 理由 1: A* 需要对节点去重，Action 无法表达“到过同一个地方”

A* 的核心机制之一是 Visited / Closed Set:

```text
这个节点我之前用更低代价到达过了，跳过。
```

考虑两个路径:

```text
路径 A: Start -> Reload -> Aim
结果 State = {Ammo=6, Aiming=true}

路径 B: Start -> Aim -> Reload
结果 State = {Ammo=6, Aiming=true}
```

两条路径的 Action 顺序不同，但最终 WorldState 完全一样。

如果节点是 State:

```text
Planner 能识别“这个 State 已经见过”，只保留 GScore 更低的路径。
```

如果节点是 Action:

```text
Reload 和 Aim 只是动作名，无法判断两条路径已经到达同一个结果状态。
```

这会导致重复展开后续所有分支，搜索空间很容易爆炸。

所以 State 是判断“是否到过同一个地方”的唯一可靠依据。

#### 理由 2: 终止条件是状态满足，不是动作执行

GOAP 的 Goal 是用状态描述的:

```text
EnemyAlive = false
```

不是用动作描述的:

```text
执行 Shoot 动作
```

因为 Shoot 可能命中，也可能没命中；也可能敌人已经被爆炸、陷阱或队友击杀。GOAP 真正关心的是目标状态是否成立。

所以终止判断天然是:

```text
if Current.State satisfies GoalState:
    return Plan
```

既然终止对象是 State，把 State 作为节点最自然。

#### 理由 3: Heuristic 的输入是 State

HScore 衡量的是“当前离 Goal 还有多远”。它比较的是两个状态:

```text
H = 当前 State 与 GoalState 之间未满足的条件数量或加权差距
```

Action 脱离上下文没有“距离 Goal 多远”的概念。

例如 `Reload` 这个 Action:

```text
如果 Goal 是 KillEnemy，Reload 可能有用。
如果 Goal 是 Escape，Reload 可能没用。
如果当前 Ammo 已满，Reload 甚至不可用。
```

只有 Action 作用在某个 State 上并生成 NextState 后，才能计算:

```text
H(NextState, GoalState)
```

所以启发式搜索的评分对象必须是 State。

#### 理由 4: 回溯需要 State 链，Action 只是链上的边

回溯结构是:

```text
GoalNode -> Parent -> Parent -> StartNode
```

每个 Node 保存:

```text
State
Parent
ParentAction
```

示例:

```text
Node0.State = StartState

Node1.State = AmmoReady=true
Node1.Parent = Node0
Node1.ParentAction = Reload

Node2.State = Aiming=true
Node2.Parent = Node1
Node2.ParentAction = Aim

Node3.State = EnemyAlive=false
Node3.Parent = Node2
Node3.ParentAction = Shoot
```

从 Goal State 往回走 Parent 链，并收集每条边上的 ParentAction:

```text
Shoot <- Aim <- Reload
```

反转后得到:

```text
Reload -> Aim -> Shoot
```

如果节点是 Action，链会变成:

```text
Reload -> Aim -> Shoot
```

表面看也能连起来，但 Action 之间没有天然父子关系。`Reload` 后能不能接 `Aim`，取决于 `Reload` 产生的中间 State 是否满足 `Aim` 的 Preconditions。也就是说，Action 之间的连接关系仍然绕不开 State。

#### 地图寻路类比

标准 A* 地图寻路:

| 地图寻路 | GOAP |
| --- | --- |
| 节点 = 格子 / 位置 | 节点 = WorldState |
| 边 = 从一个格子移动到另一个格子的动作 | 边 = Action |
| Cost = 移动距离 | Cost = Action.Cost |
| H = 当前格子到终点的估算距离 | H = 当前 State 与 Goal 的估算差距 |
| 去重 = 同一个格子不重复展开 | 去重 = 同一个 State 不重复展开 |

在地图寻路里，不会说“节点是移动动作”。移动动作只是把你从格子 A 带到格子 B 的边。

GOAP 同构:

```text
Action 只是把世界从 State A 带到 State B 的边。
```

#### 启发式搜索如何通过 WorldState 运行

具体流程:

```text
1. OpenSet 里放 Node，每个 Node 内嵌一个 State。

2. 从 OpenSet 取出 FScore 最低的 Node。
   排序对象是 Node，也就是某个 State。

3. 遍历每个 Action:
   如果 Action.Preconditions 匹配 Current.State:
       NextState = Apply(Current.State, Action.Effects)

       NextNode.G = Current.G + Action.Cost
       NextNode.H = Heuristic(NextState, GoalState)
       NextNode.F = NextNode.G + NextNode.H

       如果 NextState 没见过，或新 GScore 更小:
           OpenSet.Push(NextNode)
```

Action 在流程里出现两次:

- 作为状态转移函数: `CurrentState -> NextState`。
- 作为 ParentAction 记录在边上，用于最后回溯 Plan。

State 出现在所有关键决策点:

- OpenSet 排序。
- Visited / Closed Set 去重。
- GoalState 终止判断。
- Heuristic 估算。
- Parent 回溯起点。

所以 State 是搜索图的节点，Action 是连接节点的边。

一句话总结:

```text
A* 搜索的是状态空间，不是动作空间。
Action 是状态之间的转移规则，WorldState 才是搜索图里的真正节点。
```

### 5.8 伪代码


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

### 7.2 蓝图职责

项目是纯 C++/蓝图项目，不引入脚本语言。

蓝图适合做可视化配置和少量项目定制逻辑:

- 创建具体 Action 蓝图子类。
- 创建 Goal 蓝图或 DataAsset 配置。
- 配置 Preconditions、Effects、Cost、Priority。
- 覆写 CanRun / ReceiveStart / ReceiveTick / ReceiveAbort。
- 读取 Actor、AIController、Blackboard、AbilitySystem 等 UE 对象状态。
- 驱动动画、移动、技能、交互等已有 UE 逻辑。

蓝图 Action 示例接口:

```cpp
UCLASS(Blueprintable, Abstract)
class UGOAPAction : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GOAP")
    FName ActionId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GOAP")
    float Cost = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GOAP")
    FGOAPWorldState Preconditions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GOAP")
    FGOAPWorldState Effects;

    UFUNCTION(BlueprintNativeEvent, Category="GOAP")
    bool CanRun(AActor* Owner) const;

    UFUNCTION(BlueprintNativeEvent, Category="GOAP")
    void ReceiveStart(AActor* Owner);

    UFUNCTION(BlueprintNativeEvent, Category="GOAP")
    EGOAPActionStatus ReceiveTick(AActor* Owner, float DeltaSeconds);

    UFUNCTION(BlueprintNativeEvent, Category="GOAP")
    void ReceiveAbort(AActor* Owner);
};
```

蓝图侧 `BP_GOAPAction_Reload` 可以这样配置:

```text
ActionId = Reload
Cost = 2
Preconditions:
  HasWeapon = true
  AmmoEmpty = true
Effects:
  AmmoReady = true

CanRun:
  Owner 有武器，且当前弹药为 0

ReceiveStart:
  播放换弹 Montage

ReceiveTick:
  Montage 结束后设置弹药，返回 Success
  未结束返回 Running

ReceiveAbort:
  停止换弹 Montage
```

### 7.3 C++ 与蓝图边界

建议边界:

- C++ 负责规划器、WorldState、状态比较、状态 Hash、Plan 队列和性能控制。
- 蓝图负责具体 Action / Goal 的配置、少量运行时校验和对 UE 玩法对象的调用。
- 规划阶段尽量只读取 C++ 已缓存的 Preconditions、Effects、Cost，避免在搜索过程中频繁执行蓝图逻辑。
- 动态世界信息先由组件同步成 WorldState，再交给 C++ Planner。

不建议:

- 在 A* 每次扩展节点时执行复杂蓝图图表。
- WorldState 使用任意字符串表达式。
- 把播放动画、移动角色、修改真实属性等副作用写进 Planner。
- 让蓝图直接修改 OpenSet、Visited、Parent 链等规划器内部结构。

推荐做法:

- Action 的静态规划数据用 `EditDefaultsOnly` 配置。
- `CanRun` 只做执行前二次校验，不参与大规模搜索。
- 复杂动作执行由蓝图事件触发，但动作生命周期由 `UGOAPAgentComponent` 管理。
- 高频 AI 或性能敏感 Action 可以直接写 C++ 子类。
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
- 支持 C++ Action 类和蓝图 Action 子类注册。
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
- 蓝图用于配置和扩展 Action / Goal，规划核心保持在 C++。






