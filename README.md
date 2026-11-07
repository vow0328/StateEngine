# MainStat 表驱动状态迁移引擎

`MainStat.c` / `MainStat.h` 是表驱动状态迁移引擎本体：状态表指向自己的迁移表数组，输入触发值后，引擎匹配迁移条件、调用处理函数，再切换到目标状态表。本仓库包含引擎核心、最小移植模板和交互式数据库选择例程。

建议运行数据库选择例程，并对照 [`StateEngine_Example/状态迁移详解.md`](StateEngine_Example/状态迁移详解.md) 单步观察入口检查、页面内操作、删除和自动迁移。

## 状态表与迁移表

| 类型 | 含义 |
|---|---|
| `StateType_t` | **状态表**：保存前处理函数、自动迁移标志、迁移表条目数与数组指针、画面 ID。 |
| `StateTable_t` | **迁移表结构**：每个数组元素是一个迁移表项，保存迁移条件、后处理函数和目标状态表指针。 |

状态表的 `transTbl` 指向迁移表数组，`transNum` 表示数组条目数；迁移表项的 `nextState` 指向目标状态表。指针关系为：

```text
stateInfo.state → 当前状态表 → transTbl → 迁移表数组 → 命中表项 → nextState → 目标状态表
```

## 文件

| 文件 | 说明 |
|---|---|
| `MainStat.c` / `MainStat.h` | **引擎本体**：迁移循环、触发器匹配、状态历史栈、特殊迁移目标、临界区占位。 |
| `TypeDefine_Sample.h` | 最小类型与通用标志模板：`UI_8` / `UI_16` / `BOOL`、`TRUE/FALSE/OK/NG/YES/NO/PUBLIC`、`SCREEN_ID_NONE`。实机替换为项目公共类型头。 |
| [`StateEngine_Example/`](StateEngine_Example/) | 交互式数据库选择与模拟删除例程，演示不同层次的状态迁移。 |

`MainStat.c` / `MainStat.h` 是引擎代码；`TypeDefine_Sample.h` 只是让本仓库能独立编译、并作为移植示例，不属于引擎通用部分。

`StateEngine_Example/` 是独立可编译的例程，内含一份 `MainStat.c/.h` 副本；根目录的 `MainStat.c/.h` 是当前核心版本。例程用于演示接入方式，两者如有差异以根目录核心为准。

## 独立编译检查

在本目录执行：

```sh
gcc -std=c99 -Wall -Wextra -pedantic -c MainStat.c
```

会生成一个 `MainStat.o`，检查完可以删除。不需要 `main()`，也不需要启动文件。

## 范围触发器配置

- `MainStat.h` 提供默认的 `STATE_RANGE_MIN`、`STATE_RANGE_MAX`、`STATE_ANY`；工程可在包含该头文件前重定义。
- 引擎只在 `MainStat.h` 中声明对照表，不提供默认定义：工程侧必须定义 `TriggerRanges`。如果不使用范围匹配，迁移表项的 `trigger` 不得落在保留的范围标识区间；即使对照表全为 0，范围条件仍会匹配触发值 0。
- 对照表下标为“迁移表项的 `trigger - STATE_RANGE_MIN`”，表项为 `{min, max}`；输入触发值用于与该区间比较。
- `STATE_ANY` 是独立的无条件匹配标识，不查询范围表；迁移表项仍按数组顺序匹配，兜底表项应放在末尾。该标识值保留给迁移条件使用，不应与普通触发值重复。

## 接入真实工程

1. 提供 `UI_8`、`UI_16`、`BOOL`、`TRUE`、`FALSE`、`OK`、`NG`、`YES`、`NO`、`PUBLIC`、`SCREEN_ID_NONE`。可以用项目已有的公共类型头替换 `TypeDefine_Sample.h`，并修改 `MainStat.h` 的 include。
2. 范围触发器按上一节配置 `STATE_RANGE_MIN`、`STATE_RANGE_MAX` 并定义 `TriggerRanges`；不使用范围匹配时也要提供该表，否则链接缺少符号。配置宏必须在编译 `MainStat.c` 的过程中可见，并与定义对照表的编译单元一致，不能只在业务源文件中定义。
3. `MainStat.c` 末尾有两个空的 `PlatformEnterCritical()` / `PlatformExitCritical()`。实机删除这两个空实现，改为链接显示平台的实际临界区函数。
4. 在项目头文件中声明引擎提供的两个特殊迁移目标：

   ```c
   extern const StateType_t PreviousState;   /* 特殊状态表：取上一历史状态表指针（出栈） */
   extern const StateType_t FormEntryState;  /* 特殊状态表：取历史栈顶状态表指针（不弹栈） */
   ```

5. 工程侧定义状态表、迁移表数组、前处理函数、后处理函数、按键值和屏幕 ID。返回特定状态时，可直接把迁移表项的 `nextState` 写成目标状态表指针。
6. `StateGetHistoryTop()` 返回历史栈的有效条目数；例程用它从栈顶向前查找指定首页。
7. **不要修改 `StateEngine()` 内部的 `while (1)` 迁移循环。** 它是整个引擎的核心。

## 前处理函数与后处理函数

两个名称分别以离开的状态和进入的状态为参照，执行顺序是**先后处理，再前处理**：

1. 引擎在当前状态表关联的迁移表数组中匹配迁移条件，执行命中表项的 `onExit` 后处理函数，完成当前状态的相关处理；自迁移时也会执行。
2. 引擎将当前状态表指针切换到该迁移表项的 `nextState`，解析特殊状态表占位并保存历史，再执行目标状态表的 `onEnter` 前处理函数。自迁移也会重新执行本状态表的前处理。
3. 后处理函数的返回值作为参数传给目标状态表的前处理。若目标状态表的 `autoFlg` 为 `YES`，前处理返回值成为下一轮查询其迁移表的触发值；否则引擎返回调用方。

后处理函数属于**命中的迁移表项（`StateTable_t`）**，因此同一迁移表的不同表项可以配置不同处理；前处理函数属于**目标状态表（`StateType_t`）**。`StateEngineInit()` 只设置当前状态表指针和触发值、清空历史，不执行前处理、不自动继续迁移，也不会将初始状态表入栈；根目录核心与例程副本的初始化行为一致。

例如例程中 `DummyFunc` 是无操作占位函数，作为状态表的前处理或迁移表项的后处理时原样传递触发值；入口状态表的 `CheckDatabaseSelect` 前处理返回正常、无数据或模拟错误条件，再由其迁移表中的对应表项指定目标状态表。

**接口名称：**状态表使用 `onEnter`、`autoFlg`、`transNum`、`transTbl`；迁移表项使用 `trigger`、`onExit`、`nextState`。`SetStateTrig()` 设置当前触发值，`GetStateTrig()` 读取触发值，`GetStatePtr()` 取得当前状态表指针；`ListOf()` 初始化迁移表条目数和数组指针。范围配置使用 `STATE_RANGE_MIN/MAX`，历史记录使用 `stateHistory.states` 和 `stateHistory.top`。使用旧名称的工程需同步更新并重新编译。

## 设计约束

- 使用引擎前确保当前状态表指针非空；参与迁移的表项后处理函数和目标状态表前处理函数非空。未命中迁移表项时引擎直接返回。
- 自动迁移状态表关联的迁移表应覆盖前处理所有可能返回的条件，包括业务错误；自动迁移链应保证最终退出，避免无限循环。未命中表项时引擎直接返回，此时当前指针可能仍指向自动迁移状态表。
- `ListOf(ruleArray)` 用于初始化状态表，按顺序展开为“迁移表条目数 + 迁移表数组指针”；参数必须是数组。
- 状态历史栈深度为 25，当前实现没有栈满检查，超过深度会越界写入；接入方必须限制历史长度。
- 自动迁移状态表（`autoFlg` bit0 为 1）是过渡状态的状态表，其指针不入历史栈。
- 初始化不会保存初始状态表指针，自动迁移状态表也不入栈，因此栈顶指针不总是当前状态表指针。使用特殊回退目标前必须保证栈非空；为空时会得到空指针，后续解引用不安全。
- 引擎不负责画面绘制；画面由工程侧根据 `GetStatePtr()->screenId` 决定。

## 完整例程

数据库例程中，每次画面切换都先经过 `autoFlg = YES` 的入口或回退过渡状态表，再进入 `autoFlg = NO` 的画面状态表；打开内容、编辑、删除确认、工作画面、提示页和历史返回均遵循这一结构。页内操作直接自迁移，回退保留选择和光标，前进入口按需检查并初始化；数据仅保存在内存中。编译与运行方式见 [`StateEngine_Example/README.md`](StateEngine_Example/README.md)。
