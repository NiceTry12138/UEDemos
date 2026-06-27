# 技能系统设计总结

## 1. 设计目标

本技能系统采用 **数据驱动 + Runner 调度 + 连续内存存储运行时数据** 的设计方式。

核心目标：

- 技能本身由静态数据描述，不直接参与运行时计算。
- 技能执行由 `SkillRunner` 统一推进。
- 技能运行时数据集中存储，提升 cache 命中率。
- `SkillTask` 尽量保持无状态，通过外部传入的数据块完成计算。
- 支持时间轴任务、持续性任务、事件驱动任务。
- 支持 Montage、Hitbox、Hurtbox、事件发送、阶段切换等技能行为。
- 支持 Task 池化，减少运行时频繁分配。

---

## 2. 核心对象职责

### 2.1 SkillAsset

`SkillAsset` 是技能的静态配置资源，只描述技能，不参与运行时推进。

主要包含：

- 技能的 Stage 列表
- 每个 Stage 的时间轴任务
- 每个 Stage 的事件映射表
- 技能级事件映射表
- Task 配置数据
- Stage 配置数据
- 技能运行时数据布局描述
- 编译后的 offset / size / alignment 信息

`SkillAsset` 不保存任何运行状态。

---

### 2.2 SkillComponent

`SkillComponent` 是角色或实体上的技能运行容器。

主要职责：

- 管理当前实体拥有的技能
- 管理多个 `SkillRunner`
- 为技能运行分配和回收运行时内存
- 提供外部上下文，例如 Owner、Avatar、CombatContext
- 作为技能系统和角色系统、战斗系统之间的连接点

技能运行时数据放在 `SkillComponent` 管理的连续内存中，而不是单独创建 `SkillInstance`。

---

### 2.3 SkillRunner

`SkillRunner` 是技能的真正执行者。

主要职责：

- 推进技能时间轴
- 管理当前 Stage
- 管理当前技能时间
- 调度时间轴上的 Task
- 调度事件触发的 Task
- 维护事件队列
- 维护持续性 Task 列表
- 管理 `bCreateInstance = true` 的 Task 实例生命周期
- 处理 Stage 切换请求
- 在技能结束后回收运行时资源

`SkillRunner` 是技能流程的主控者，Stage 不直接推进时间轴。

---

### 2.4 SkillStage

`SkillStage` 表示技能的一个阶段。

Stage 不推进时间轴，但定义该阶段的时间域和行为配置。

主要包含：

- Stage 持续时间
- 时间轴 Task 配置
- Stage 级事件映射表
- StageData 布局
- 默认下一个 Stage
- 是否允许打断
- 是否允许切换到其他 Stage
- Stage 切换规则

Stage 的职责是描述当前阶段内可以发生什么，而不是驱动技能运行。

---

## 3. SkillTask 设计

### 3.1 Task 类型设计原则

Task 不通过粗暴的枚举值区分类型，避免新使用者理解成本过高。

采用继承结构区分：

```cpp
class USkillTask
{
public:
    virtual void Reset() {}
    virtual void OnExecute(FSkillTaskContext& Context) {}
};
```

```cpp
class USustainSkillTask : public USkillTask
{
public:
    virtual void OnStart(FSkillTaskContext& Context) {}
    virtual void OnTick(FSkillTaskContext& Context, float DeltaTime) {}
    virtual void OnEnd(FSkillTaskContext& Context, ESkillTaskEndReason EndReason) {}
};
```

使用者根据任务行为选择继承：

- 瞬时任务继承 `USkillTask`
- 持续性任务继承 `USustainSkillTask`

这样比通过 `TaskMode`、`TaskType` 枚举区分更直观。

---

### 3.2 USkillTask

`USkillTask` 表示瞬时任务或普通动作任务。

适合：

- 发送事件
- 播放一次性效果
- 触发一次伤害检测
- 切换 Stage 请求
- 修改角色状态
- 播放瞬时表现
- 打开某个标记

核心接口：

```cpp
class USkillTask
{
public:
    virtual void Reset();
    virtual void OnExecute(FSkillTaskContext& Context);
};
```

`OnExecute` 在任务被触发时执行一次。

---

### 3.3 USustainSkillTask

`USustainSkillTask` 表示持续性任务。

适合：

- 持续 Hitbox / Hurtbox
- 持续位移
- 持续锁输入
- 持续霸体
- 持续检测
- 持续引导
- 持续播放或监听 Montage 状态

核心接口：

```cpp
class USustainSkillTask : public USkillTask
{
public:
    virtual void OnStart(FSkillTaskContext& Context);
    virtual void OnTick(FSkillTaskContext& Context, float DeltaTime);
    virtual void OnEnd(FSkillTaskContext& Context, ESkillTaskEndReason EndReason);
};
```

生命周期：

```text
OnStart -> OnTick... -> OnEnd
```

持续性任务可以由时间轴配置开始时间和结束时间。

---

## 4. Task 生命周期

### 4.1 瞬时 Task

瞬时 Task 只在触发时调用一次：

```text
Trigger -> OnExecute
```

它可以来自：

- 时间轴上的瞬时点
- 事件映射表
- 其他 Task 触发
- Stage 进入时触发
- 外部系统触发

---

### 4.2 持续性 Task

持续性 Task 有完整生命周期：

```text
StartTime -> OnStart
DuringTime -> OnTick
EndTime -> OnEnd
```

如果 Stage 提前切换、技能被打断、角色死亡或技能结束，也必须调用 `OnEnd`。

建议定义结束原因：

```cpp
enum class ESkillTaskEndReason
{
    NaturalEnd,
    StageChanged,
    SkillInterrupted,
    SkillFinished,
    OwnerDestroyed
};
```

这样可以保证持续性任务正确清理自身影响。

---

## 5. bCreateInstance 规则

每个 `SkillTask` 拥有 `bCreateInstance` 属性。

### 5.1 bCreateInstance = false

表示该 Task 是无状态任务。

运行时直接调用 CDO 或共享对象。

约束：

- 不允许写入 Task 成员变量
- Task 内部必须保持无状态
- 运行状态必须写入外部传入的数据结构
- 所有计算依赖 `SkillTaskDataStruct`、`StageDataStruct`、`SkillDataStruct` 或 Context

适合大部分纯逻辑任务。

---

### 5.2 bCreateInstance = true

表示该 Task 需要运行时实例。

运行时从对象池中获取 Task 实例，技能结束后 Reset 并回收到池中。

约束：

- 允许持有临时状态
- 必须实现 `Reset`
- Runner 结束时统一回收
- 不允许未 Reset 的对象再次被复用
- 不允许跨 Runner 共享同一个运行时实例

适合确实需要对象内部状态的特殊 Task。

---

## 6. 运行时数据布局

### 6.1 数据集中存储

技能运行时数据不分散存储在多个对象中，而是由 `SkillComponent` 分配一块连续内存。

建议结构：

```text
SkillRuntimeMemory
 ├─ SkillData
 ├─ StageData[]
 └─ TaskData[]
```

这样可以提升数据局部性，减少运行时对象访问开销。

---

### 6.2 TaskDataStruct

每个 `SkillTask` 定义自己的运行时数据结构。

示例：

```cpp
struct FPlayMontageTaskData
{
    float StartedTime;
    bool bHasStarted;
};
```

Task 自身尽量不保存状态，而是从 Context 中取得自己的数据块：

```cpp
FPlayMontageTaskData* Data = Context.GetTaskData<FPlayMontageTaskData>();
```

---

### 6.3 StageDataStruct

每个 `SkillStage` 可以定义该 Stage 内 Task 共享的数据。

适合存储：

- 当前 Stage 的共享状态
- 当前 Stage 的命中记录
- 当前 Stage 的输入缓存
- 当前 Stage 的派生状态
- 当前 Stage 的临时运行标记

---

### 6.4 SkillDataStruct

每个技能可以定义技能级运行时数据。

适合存储：

- 当前技能整体状态
- 已命中目标列表
- 技能全局计数
- 技能释放参数
- 技能上下文缓存

---

## 7. Offset 与 Layout

### 7.1 使用 index + offset 访问数据

运行时通过 index 和 offset 获取对应数据块。

例如：

```cpp
void* SkillData = BasePtr + SkillDataOffset;
void* StageData = BasePtr + StageDataOffset;
void* TaskData  = BasePtr + TaskDataOffset;
```

这种方式可以保留，不需要额外为每个 Task 创建运行时对象。

---

### 7.2 编译阶段生成 Layout

虽然运行时使用 offset 访问，但 offset 不建议在运行时临时推导。

建议在技能编译阶段生成 Layout：

```cpp
struct FSkillRuntimeLayout
{
    int32 TotalSize;
    int32 SkillDataOffset;
    int32 SkillDataSize;

    TArray<FStageDataLayout> StageLayouts;
    TArray<FTaskDataLayout> TaskLayouts;
};
```

```cpp
struct FTaskDataLayout
{
    int32 TaskIndex;
    int32 DataOffset;
    int32 DataSize;
    int32 Alignment;
};
```

运行时只读取编译好的 Layout。

---

### 7.3 Alignment

连续内存布局必须考虑对齐。

生成 offset 时需要执行：

```cpp
Offset = Align(Offset, Alignment);
```

每个数据块都应该记录：

- Offset
- Size
- Alignment
- TypeId
- DebugName

这样可以避免跨平台对齐问题，也方便调试和校验。

---

## 8. 技能时间轴

技能时间轴由 `SkillRunner` 推进。

Stage 只定义时间域：

```text
Stage Duration
Timeline Tasks
Task StartTime
Task EndTime
```

时间轴上的任务分为：

- 到点执行的 `USkillTask`
- 有开始和结束时间的 `USustainSkillTask`

示例：

```text
Stage_Attack
 ├─ 0.00s: PlayMontageTask.OnExecute
 ├─ 0.15s: HitboxTask.OnStart
 ├─ 0.30s: HitboxTask.OnEnd
 └─ 0.50s: RequestNextStageTask.OnExecute
```

---

## 9. 事件系统

### 9.1 事件映射表

事件触发时，根据事件映射表执行对应 Task。

建议分为两层：

```text
Skill-level EventMap
Stage-level EventMap
```

优先级：

```text
Stage EventMap > Skill EventMap
```

用途：

- Skill-level EventMap：处理通用事件，例如技能取消、被打断、角色死亡。
- Stage-level EventMap：处理当前阶段特有事件，例如命中派生、连段输入、蓄力完成。

---

### 9.2 事件触发时机

事件支持三种触发类型：

```text
Immediate
FrameEnd
NextFrameBegin
```

含义：

- `Immediate`：立即插入当前事件执行流程。
- `FrameEnd`：当前帧尾统一执行。
- `NextFrameBegin`：下一帧开始时执行。

---

### 9.3 事件执行顺序

建议 Runner 每帧执行流程：

```text
FrameBegin:
  1. 执行上一帧遗留的 NextFrameBegin 事件

Tick:
  2. 推进当前 Stage 时间轴
  3. 触发到点的瞬时 Task
  4. 启动或结束持续性 Task
  5. Tick 当前持续性 Task
  6. 收集本帧事件

Immediate:
  7. 立即事件插入当前事件队列执行

FrameEnd:
  8. 执行 FrameEnd 事件
  9. 处理 PendingStageChange
  10. 清理结束的 Task
```

---

## 10. Stage 切换

Task 可以请求切换 Stage，但不应该直接修改 Runner 当前 Stage。

推荐方式：

```cpp
Context.RequestStageChange(TargetStageIndex, Reason);
```

Runner 收到请求后记录为 `PendingStageChange`。

Stage 切换必须等当前事件队列执行完成后统一处理。

这样可以避免：

- 事件执行中途 Stage 被切换
- StageData 指针失效
- 当前事件队列和新 Stage 状态混乱
- 持续性 Task 未正确结束

---

### 10.1 Stage 切换请求

建议定义：

```cpp
struct FStageChangeRequest
{
    int32 TargetStageIndex;
    int32 Priority;
    EStageChangeReason Reason;
};
```

Runner 根据规则判断是否接受：

- 当前 Stage 是否允许切换
- 目标 Stage 是否合法
- 当前是否处于事件执行中
- 是否覆盖已有切换请求
- 新请求优先级是否更高

---

## 11. Montage Task

播放 Montage 可以作为一个普通 `SkillTask`。

但 Montage 不应该成为技能主流程控制器。

建议规则：

```text
SkillRunner 是主时钟
MontageTask 是表现层 Task
Montage Notify 只发送事件
EventMap 决定后续技能逻辑
```

例如：

```text
MontageNotify_HitFrame -> SendSkillEvent(HitFrame)
MontageNotify_CancelWindow -> SendSkillEvent(CancelWindowOpen)
```

然后由 Stage EventMap 或 Skill EventMap 决定执行哪些 Task。

这样技能逻辑不会强绑定动画资源。

---

## 12. 技能编译阶段

建议将编辑态技能数据编译成运行态数据。

```text
SkillAsset 编辑态
        ↓ Compile
CompiledSkillAsset 运行态
```

编译阶段处理：

- Stage 合法性校验
- Task 时间合法性校验
- EventMap 校验
- TaskIndex 分配
- StageIndex 分配
- 数据 Layout 生成
- Offset 计算
- Alignment 处理
- 持续性 Task 开始和结束时间排序
- DebugName / TypeId 记录
- 无效配置报错

运行时只读取编译后的结果，不做复杂推导。

---

## 13. 推荐整体结构

```text
SkillAsset
 ├─ SkillGlobalDataLayout
 ├─ SkillEventMap
 ├─ SkillStages[]
 │   ├─ Duration
 │   ├─ TimelineTasks[]
 │   ├─ StageEventMap
 │   ├─ StageDataLayout
 │   └─ StageChangeRules
 └─ RuntimeLayout

SkillComponent
 ├─ RuntimeMemoryPool
 ├─ SkillRunners[]
 └─ ExternalContext

SkillRunner
 ├─ CurrentSkill
 ├─ CurrentStageIndex
 ├─ CurrentTime
 ├─ RuntimeMemory
 ├─ ActiveSustainTasks
 ├─ InstancedTasks
 ├─ EventQueue
 └─ PendingStageChange

USkillTask
 ├─ bCreateInstance
 ├─ Reset
 └─ OnExecute

USustainSkillTask : USkillTask
 ├─ OnStart
 ├─ OnTick
 └─ OnEnd
```

---

## 14. 设计结论

最终设计可以概括为：

- `SkillAsset` 负责描述技能。
- `SkillComponent` 负责管理技能运行时数据。
- `SkillRunner` 负责推进技能执行。
- `SkillStage` 负责定义阶段时间域和事件规则。
- `USkillTask` 负责瞬时行为。
- `USustainSkillTask` 负责持续性行为。
- 事件系统负责非时间轴驱动的技能逻辑。
- Stage 切换通过请求机制延迟处理。
- 技能运行时数据通过连续内存和 Layout 访问。
- `bCreateInstance` 用于区分无状态 Task 和需要实例化的有状态 Task。
- Montage 只是 Task，不反向控制技能主流程。

该方案整体上是一个 **Runner 驱动、数据集中、Task 可扩展、事件可派生、内存 cache-friendly** 的技能系统架构。
