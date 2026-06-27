# GOAP WorldState 设计讨论

修改内容: 讨论 WorldState Key 注册、编辑期下拉选择、运行期 Index/Mask 状态表示方案。

## 1. 结论

这个方向是对的。

核心思路成立:

```text
编辑期: FName Key + 类型 + 默认值，方便配置和下拉选择
运行期: KeyIndex + 紧凑数组 + Mask，方便规划循环 O(1) 访问
```

GOAP Planner 运行时会高频做:

- Preconditions 匹配。
- Effects 应用。
- State Hash。
- Visited / ClosedSet 查重。

所以不能在规划循环里反复用 `TMap<FName, Value>` 查找。编辑层保留 `FName`，运行层编译成 `Index`，是正确方向。

## 2. Settings 定义全局 State Key

`UGOAPSettings` 用来登记所有合法 State Key:

```cpp
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="GOAP Settings"))
class GOAPRUNTIME_API UGOAPSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Config, Category="State Keys")
    TArray<FGOAPStateKeyDef> StateKeys;
};
```

每个 Key 定义:

```cpp
USTRUCT(BlueprintType)
struct FGOAPStateKeyDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName Key;

    UPROPERTY(EditAnywhere)
    EGOAPValueType Type = EGOAPValueType::Bool;

    UPROPERTY(EditAnywhere)
    FGOAPStateValue DefaultValue;

    UPROPERTY(EditAnywhere, meta=(MultiLine=true))
    FString Comment;
};
```

这个设计好处:

- 所有 GOAP 状态集中注册，避免 Action 里手填错 Key。
- Detail 面板可以做下拉选择。
- 可以做类型校验。
- 可以为运行层构建稳定 `Key -> Index`。

## 3. 需要注意的问题

### 3.1 `FGOAPStateValue` 不建议长期保存独立 Type

你现在的设计:

```cpp
struct FGOAPStateValue
{
    EGOAPValueType Type;
    int32 IntValue;
    FName NameValue;
};
```

问题: `KeyDef.Type` 和 `Value.Type` 可能不一致。

例如:

```text
Key = Ammo
KeyDef.Type = Int
Value.Type = Bool
```

这会制造双重真相。

建议:

- `KeyDef.Type` 是唯一权威类型。
- `FGOAPStateValue` 可以保留 `Type` 作为编辑期显示辅助，但保存/编译前必须校验。
- 更干净的做法是运行层不存 `Type`，只按 `KeyIndex` 去查 `StateKeyDefs[Index].Type`。

运行层推荐:

```cpp
struct FGOAPRuntimeValue
{
    int32 IntValue = 0;   // Bool / Int / Enum 都可放这里
    FName NameValue;
};
```

类型由 `KeyIndex` 对应的 `FGOAPStateKeyDef.Type` 决定。

### 3.2 `ConditionSet` 比 `WorldState` 更准确

Action 的 InState / OutState 不应该表达完整 WorldState，而是表达“关心哪些条件”和“改哪些结果”。

所以这个命名更准确:

```cpp
FGOAPConditionSet Preconditions;
FGOAPConditionSet Effects;
```

或者统一叫 Patch:

```cpp
struct FGOAPStatePatch
{
    TArray<FGOAPCondition> Items;
};
```

关键是必须有 Mask:

```text
Value = 具体值
Mask = 这个 Key 是否参与匹配/应用
```

否则无法区分:

```text
HasWeapon = false
```

和:

```text
没有填写 HasWeapon 条件
```

### 3.3 `uint64 DirtyMask` 只能支持 64 个 Key

如果 GOAP State Key 数量确定少于等于 64，`uint64` 很快，很适合第一版。

但要明确限制:

```text
StateKeys.Num() <= 64
```

如果未来可能超过 64，建议一开始就封装 Mask:

```cpp
struct FGOAPStateMask
{
    TArray<uint64> Words;

    bool Contains(int32 Index) const;
    void Add(int32 Index);
};
```

这样第一版内部也可以只用一个 `uint64`，但接口不被 64 位限制绑死。

### 3.4 Settings 里的静态映射要小心生命周期

`PostEditChangeProperty` 只在编辑器变更时跑，且需要:

```cpp
#if WITH_EDITOR
virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
```

运行时不能依赖编辑器回调。

建议:

- `UGOAPSettings` 只负责保存配置。
- 单独做一个 Registry 缓存 `Key -> Index`。
- Registry 在模块启动、PIE 开始或第一次访问时从 `GetDefault<UGOAPSettings>()` 构建。
- Settings 变更时通知 Registry 重建。

例如:

```cpp
class FGOAPStateRegistry
{
public:
    void RebuildFromSettings(const UGOAPSettings& Settings);
    int32 GetKeyIndex(FName Key) const;
    const FGOAPStateKeyDef* FindKey(FName Key) const;

private:
    TArray<FGOAPStateKeyDef> KeyDefs;
    TMap<FName, int32> KeyToIndex;
};
```

这样比把大量静态状态塞进 `UDeveloperSettings` 更稳。

### 3.5 Key 重命名会影响已有 Action 资产

Action 里保存的是 `FName Key`。如果 Settings 里把 `EnemyVisible` 改名为 `CanSeeEnemy`，已有 Action 里的旧 Key 不会自动更新。

建议 `FGOAPStateKeyDef` 加稳定 ID:

```cpp
UPROPERTY(VisibleAnywhere)
FGuid Id;

UPROPERTY(EditAnywhere)
FName Key;
```

第一版也可以不做 ID，但至少要提供校验工具:

- 检查 Action 是否引用了不存在的 Key。
- 检查 Condition.Value 类型是否匹配 KeyDef.Type。
- 检查重复 Key。
- 检查 DefaultValue 类型是否匹配。

## 4. 推荐第一版落地方案

第一版可以保守一些:

1. `UGOAPSettings` 定义全局 `StateKeys`。
2. 限制 `StateKeys.Num() <= 64`。
3. `FGOAPCondition` 编辑期保存 `FName Key + FGOAPStateValue Value`。
4. Action 保存:

```cpp
FGOAPConditionSet Preconditions;
FGOAPConditionSet Effects;
```

5. BeginPlay / RegisterAction 时编译成运行时数据:

```cpp
struct FGOAPCompiledConditionSet
{
    FGOAPRuntimeState Values;
    uint64 CareMask = 0;
};
```

6. Planner 只吃 compiled 数据:

```cpp
bool Matches(const FGOAPCompiledConditionSet& Conditions) const;
void Apply(const FGOAPCompiledConditionSet& Effects);
```

7. DetailCustomization 做下拉和类型过滤。

## 5. 建议命名

当前命名:

```text
InState / OutState
```

容易让人误解为完整输入状态和完整输出状态。

更推荐:

```text
Preconditions
Effects
```

或:

```text
RequiredState
ResultState
```

GOAP 术语里 `Preconditions / Effects` 最标准。

## 6. 最小验证标准

实现前建议先定义这些可验证点:

- Settings 里重复 Key 会报错。
- Action 选择 Key 时只能从 Settings 下拉。
- Action 里 Value 类型必须匹配 KeyDef.Type。
- `HasWeapon=false` 和“不关心 HasWeapon”能正确区分。
- 两个不同 Action 顺序到达同一 RuntimeState 时，Hash 和 Equals 能识别为同一状态。
- `StateKeys.Num() > 64` 时有明确报错或自动扩展 Mask。

## 7. 最终判断

这个方案可以作为 GOAP WorldState 的基础。

建议调整点:

- `FGOAPStateValue.Type` 不要成为第二套权威类型。
- `ConditionSet` 运行期必须带 Mask。
- `uint64 Mask` 要明确 64 Key 限制，或封装成可扩展 Mask。
- `UDeveloperSettings` 不直接承担静态 Registry 生命周期，单独建 `FGOAPStateRegistry` 更稳。
- Action 字段使用 `Preconditions / Effects` 命名。

## 8. 第二轮讨论结论

### 8.1 已确认点

#### KeyDef.Type 是权威类型

情况说明:

`KeyDef.Type` 用来校验所有引用这个 Key 的 Value 是否合法。核心目标是避免同一个 State 在不同 Action 里被当成不同类型使用。

例如错误情况:

```text
ActionA: EnemyVisible = true   // Bool
ActionB: EnemyVisible = 1      // Int
```

修改理由:

GOAP 的状态比较、Hash、去重都依赖“同一个 Key 有稳定语义”。如果类型不稳定，同一个 Key 的值无法可靠比较，Planner 会出现隐藏错误。

结论:

- `FGOAPStateKeyDef.Type` 是唯一权威类型。
- `FGOAPStateValue.Type` 如果保留，只能作为编辑显示辅助。
- 编译 Condition / Effect 时必须用 `KeyDef.Type` 校验 Value。
- 运行层比较时不信任 Value 自带 Type。

#### InState / OutState 改为 Preconditions / Effects

情况说明:

Action 里填的不是完整 WorldState，而是“需要满足的条件”和“执行后的状态修改”。

修改理由:

`InState / OutState` 容易误导为完整输入状态和完整输出状态。GOAP 标准术语是 `Preconditions / Effects`，表达更准确，也方便后续看论文、工具和已有实现。

结论:

Action 字段命名使用:

```cpp
FGOAPConditionSet Preconditions;
FGOAPConditionSet Effects;
```

## 9. 其他设计点判断

### 9.1 是否保留 `FGOAPStateValue.Type`

情况说明:

`FGOAPStateValue` 同时放 `Type`、`IntValue`、`NameValue`，编辑器显示会方便。比如 Details 面板可以知道当前 Value 应该显示 Bool、Int 还是 Name。

但它和 `KeyDef.Type` 会形成重复信息。

修改理由:

重复信息最大风险是不同步。比如 Key 是 Bool，但 Value 存了 Int。后续每个比较函数都要处理“信谁”的问题，复杂度上升。

结论:

建议分两层:

```cpp
// 编辑层，可带 Type，服务 Details 显示
struct FGOAPStateValue
{
    EGOAPValueType Type;
    int32 IntValue;
    FName NameValue;
};

// 运行层，不带 Type，类型从 KeyIndex 查 Registry
struct FGOAPRuntimeValue
{
    int32 IntValue;
    FName NameValue;
};
```

规则:

- 编辑层 `Value.Type` 可存在，但保存/编译时必须被 `KeyDef.Type` 覆盖或校验。
- 运行层不存 Type。
- Planner 只使用运行层数据。

如果想更简单，第一版也可以让 `FGOAPStateValue` 保留 Type，但所有校验都以 `KeyDef.Type` 为准。

### 9.2 是否使用 `uint64 Mask`

情况说明:

Mask 不是优化项，而是语义必需项。因为必须区分:

```text
HasWeapon = false
```

和:

```text
不关心 HasWeapon
```

只靠 `Values` 数组无法表达“不关心”。

修改理由:

Preconditions 匹配时只能检查 CareMask 里的 Key。Effects 应用时只能写 EffectMask 里的 Key。否则 DefaultValue 会被误认为显式条件或显式效果。

结论:

必须有 Mask。

第一版建议:

```cpp
uint64 CareMask;
uint64 EffectMask;
```

并明确限制:

```text
StateKeys.Num() <= 64
```

理由: GOAP 初期 Key 数一般不会超过 64，`uint64` 实现简单、性能好、调试直观。

如果后续需求超过 64，再升级为:

```cpp
FGOAPStateMask { TArray<uint64> Words; }
```

但对外接口最好不要到处裸传 `uint64`，可以先包一层轻量结构，方便未来扩展。

### 9.3 是否需要单独 `FGOAPStateRegistry`

情况说明:

`UDeveloperSettings` 适合保存配置，不适合承担运行期全局索引缓存。`PostEditChangeProperty` 只在编辑器属性变化时触发，打包运行、模块加载、PIE 重启时都不能只依赖它。

修改理由:

运行期 Planner 需要稳定、快速、随时可用的 `Key -> Index` 映射。这个映射有生命周期问题:

- 模块启动时要构建。
- Settings 改动时要重建。
- PIE 重启时要保持正确。
- 打包运行时没有编辑器回调。

把这些逻辑放进 `UDeveloperSettings` 会让 Settings 同时负责“配置存储”和“运行期缓存”，职责混在一起。

结论:

建议做单独 Registry:

```cpp
class FGOAPStateRegistry
{
public:
    void RebuildFromSettings(const UGOAPSettings& Settings);
    int32 GetKeyIndex(FName Key) const;
    const FGOAPStateKeyDef* FindKey(FName Key) const;

private:
    TArray<FGOAPStateKeyDef> KeyDefs;
    TMap<FName, int32> KeyToIndex;
};
```

`UGOAPSettings` 只保存 `StateKeys`。

`GOAPRuntime` 模块或子系统持有 `FGOAPStateRegistry`。

### 9.4 是否需要稳定 ID / FGuid

情况说明:

如果 Action 保存的是 `FName Key`，重命名 Key 会导致已有 Action 引用失效。

例如:

```text
EnemyVisible -> CanSeeEnemy
```

已有 Action 仍保存 `EnemyVisible`，下次编译时找不到 Key。

修改理由:

稳定 ID 能解决重命名问题，但会增加编辑器工具复杂度。需要处理 ID 生成、复制、重复 ID、显示名和真实 ID 映射。

结论:

第一版不建议上 FGuid。

理由:

- 当前项目是测试 UE 项目，先控制复杂度。
- GOAP Key 数量早期不会很多。
- 重命名可以通过校验工具暴露出来。

第一版必须做校验:

- Action 引用了不存在的 Key，要报错。
- Settings 里重复 Key，要报错。
- Value 类型不匹配 KeyDef.Type，要报错。

后续如果 GOAP 资产规模变大，再加 `FGuid Id` 做稳定引用。

### 9.5 ConditionSet 和 RuntimeState 是否分离

情况说明:

编辑层 `FGOAPConditionSet` 是稀疏数据，只保存用户填写的几条条件。

运行层 `FGOAPRuntimeState` 是完整数组，大小等于 `StateKeys.Num()`。

两者用途不同。

修改理由:

编辑层要友好:

```text
Key 下拉 + Value 编辑
```

运行层要快:

```text
Values[Index] + Mask
```

如果混成一个结构，要么编辑器难用，要么运行期慢。

结论:

保留两层结构:

```cpp
// 编辑层
FGOAPConditionSet
{
    TArray<FGOAPCondition> Conditions;
}

// 编译层 / 运行层
FGOAPCompiledConditionSet
{
    TArray<FGOAPRuntimeValue> Values;
    uint64 Mask;
}
```

Action 资产保存编辑层。Agent 注册 Action 或 BeginPlay 时编译成运行层。

Planner 只读运行层。

### 9.6 DefaultValue 怎么用

情况说明:

`DefaultValue` 可以用来初始化完整 RuntimeState。比如 Agent 刚创建时，所有 StateKey 都有默认值。

但 `DefaultValue` 不能自动成为每个 Action 的 Preconditions 或 Effects。

修改理由:

Action 没填某个 Key，语义是“不关心 / 不修改”，不是“使用默认值”。如果把 DefaultValue 混入 Condition，会造成误匹配。

结论:

`DefaultValue` 只用于:

- 初始化 Agent 当前 WorldState。
- 编辑器显示默认值。
- 校验 Value 类型。

`Preconditions / Effects` 是否包含某 Key，只看 Mask。

### 9.7 Bool 是否需要单独 bitmask 存储

情况说明:

Bool 值可以压成 bitmask，速度更快，内存更小。但目前 StateValue 还支持 Int / Name。

修改理由:

第一版过早拆 BoolMask、IntValues、NameValues 会增加实现复杂度，也会让 Hash / Apply / Debug 更麻烦。

结论:

第一版不建议单独优化 Bool bitmask。

先统一用:

```cpp
TArray<FGOAPRuntimeValue> Values;
uint64 Mask;
```

等 Planner 性能有数据后，再考虑 Bool 专用 bitset。

## 10. 当前推荐版本

最终建议版本:

```cpp
USTRUCT(BlueprintType)
struct FGOAPStateKeyDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName Key;

    UPROPERTY(EditAnywhere)
    EGOAPValueType Type = EGOAPValueType::Bool;

    UPROPERTY(EditAnywhere)
    FGOAPStateValue DefaultValue;

    UPROPERTY(EditAnywhere, meta=(MultiLine=true))
    FString Comment;
};

USTRUCT(BlueprintType)
struct FGOAPStateValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    EGOAPValueType Type = EGOAPValueType::Bool; // 编辑显示辅助，KeyDef.Type 才是权威

    UPROPERTY(EditAnywhere)
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere)
    FName NameValue;
};

USTRUCT(BlueprintType)
struct FGOAPCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(GetOptions="GetRegisteredStateKeys"))
    FName Key;

    UPROPERTY(EditAnywhere)
    FGOAPStateValue Value;
};

USTRUCT(BlueprintType)
struct FGOAPConditionSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<FGOAPCondition> Conditions;
};

struct FGOAPRuntimeValue
{
    int32 IntValue = 0;
    FName NameValue;
};

struct FGOAPCompiledConditionSet
{
    TArray<FGOAPRuntimeValue> Values;
    uint64 Mask = 0;
};
```

Action 使用:

```cpp
UPROPERTY(EditAnywhere, Category="GOAP")
FGOAPConditionSet Preconditions;

UPROPERTY(EditAnywhere, Category="GOAP")
FGOAPConditionSet Effects;
```

第一版限制:

```text
StateKeys.Num() <= 64
```

必须有编译/校验步骤:

```text
FName Key -> KeyIndex
Value.Type 校验 KeyDef.Type
Conditions -> Values + Mask
重复 Key 检查
未知 Key 检查
```
