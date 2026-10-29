# STM32 表驱动状态迁移引擎（MainStat）

`MainStat.h` / `MainStat.c` 是状态迁移引擎本体。工程侧只提供基础类型、触发器定义、状态表和业务回调；引擎本身不含任何业务内容。

> 本目录是独立可编译的教学例程，内含一份 `MainStat.c/.h` 副本；根目录的 `MainStat.c/.h` 是当前核心版本。两者如有差异，以根目录核心为准。

## 1. 设计目标

- 状态与迁移关系全部由常量表描述，新增画面/流程只改表，不改引擎。
- 一次 `StateEngine(keycode)` 调用可以连续执行多个自动迁移状态，直到遇到非自动状态或没有匹配的迁移条目。
- 状态历史由引擎统一维护，支持返回上一状态、返回当前状态；返回特定状态由工程侧在表里直接指定目标。
- 引擎的 `while (1)` 迁移循环是核心，接入工程时不要修改。

## 2. 文件职责

| 文件 | 职责 |
|---|---|
| `MainStat.h` | 对外部分：数据结构、状态描述宏、引擎接口。只包含外部需要知道的内容。 |
| `MainStat.c` | 引擎实现：迁移循环、触发器匹配、状态历史栈、特殊迁移目标（`g_previousState` / `g_reenterState`）、临界区占位。栈结构、栈顶变量、内部函数均为文件私有。 |
| `TypeDefine_Sample.h` | 工程基础类型与通用标志：`UI_8/UI_16/BOOL`、`TRUE/FALSE/OK/NG/YES/NO/PUBLIC`、`SCREEN_ID_NONE`。实机替换为工程公共类型头。 |
| `TriggerDefine_Sample.h/.c` | 工程侧触发器配置：范围触发器标识 `STATE_RANGE_MIN/STATE_RANGE_MAX/STATE_RANGE_ANY`、模拟按键与井字棋按键 `KEY_*`、范围对照表 `g_triggerRanges`。 |
| `Action_StateTransition_Sample.h/.c` | 井字棋状态表、棋盘/胜负/人机 AI 逻辑，以及给屏幕层读取棋盘数据的只读接口。状态表后紧跟对应的状态对象；自动迁移状态的 `screenId` 使用 `SCREEN_ID_NONE`。实机替换为项目自己的迁移表文件。 |
| `Screen_StateTransition_Sample.h/.c` | 屏幕层：启动时切 UTF-8、清屏、按键解码、按 `screenId` 绘制画面。与 `Action_...` 对应。Windows 用 `_getch`，其它平台按行输入。 |
| `main.c` | 交互式井字棋入口：初始化屏幕 → 读键 → 调引擎 → 按 `screenId` 渲染。 |

## 3. 数据结构与宏

### 3.1 迁移条目 `StateTable_t`

| 字段 | 含义 |
|---|---|
| `trigger` | 触发值；或 `STATE_RANGE_MIN`～`STATE_RANGE_MAX` 之间的范围触发器标识 |
| `afFuncPtr` | 退出状态时处理函数，入参为本次触发值，返回值为 `UI_16` |
| `nextState` | 迁移目标状态；可以是普通状态，也可以是 `&g_previousState` / `&g_reenterState` |

### 3.2 状态 `StateType_t`

| 字段 | 含义 |
|---|---|
| `bfFuncPtr` | 进入状态时的前处理函数，入参为上一个函数的返回值，返回值可作为下一轮触发值 |
| `autoFlg : 2` | 自动迁移标志，`GetAutolFlg()` 使用 bit0：为 1 表示自动迁移状态 |
| `transitionCount : 6` | 迁移条目数量（最大 63）。位域不能取地址，引擎内部会先复制到 `UI_8` 再传入 `StateFindTable()` |
| `atTable` | 迁移条目数组 |
| `screenId` | 画面 ID，由工程侧使用 |

### 3.3 全局状态 `stateInfo`

```c
typedef struct StateInfo
{
    UI_16 trigger;                /* 当前触发值 */
    const StateType_t *state; /* 当前状态 */
} StateInfo;

extern StateInfo stateInfo;
```

### 3.4 状态描述宏

```c
#define STATESETTRIGGER(t) (stateInfo.trigger = (UI_16)(t))
#define STATE_GET_TRIGGER()  ((UI_16)stateInfo.trigger)
#define STATE_CURRENT()   (stateInfo.state)
#define STATE_RULES(tbl)     (UI_8)(sizeof(tbl) / sizeof((tbl)[0])), (tbl)
```

`STATE_RULES` 只做一件事：把数组拆成“条目数 + 数组指针”两个初始化参数，供 `StateType_t` 的 `transitionCount` 和 `atTable` 使用。

## 4. 引擎执行流程

`StateEngineInit(initialState)`：

1. `stateInfo.trigger = 0`
2. `stateInfo.state = initialState`
3. 清空状态栈
4. 执行一次 `initialState->bfFuncPtr(0)`，返回值写入 `stateInfo.trigger`（首次调用 `StateEngine()` 时会被传入的按键值覆盖）

`StateEngine(keycode)`：

1. `STATESETTRIGGER(keycode)`
2. 进入 `while (1)`：
   1. 取当前状态的迁移表和条目数
   2. `StateFindTable()` 查找匹配条目；没有匹配则直接返回（本次按键被忽略）
   3. 临界区内执行条目的 `afFuncPtr(STATE_GET_TRIGGER())`
   4. `stateInfo.state = nextState`
   5. `StateEngineRecovery()` 处理特殊迁移目标
   6. `SaveHistory()` 记录迁移后的状态
   7. 临界区内执行目标状态的 `bfFuncPtr(afFuncPtr 的返回值)`
   8. 目标状态 `autoFlg` bit0 为 0 → 返回；为 1 → 把 `bfFuncPtr` 的返回值设为下一轮触发值，继续循环

临界区通过 `PlatformEnterCritical()` / `PlatformExitCritical()` 包裹两个回调的调用。例程中是空实现，实机替换为显示平台的保护函数。

画面绘制不在引擎内：例程在每次 `StateEngine()` 返回后读取 `STATE_CURRENT()->screenId`，调用 `Screen_StateTransition_Sample.c` 中的 `TttRenderCurrentScreen()` 绘制。状态入口回调只更新棋盘、光标、菜单选中项等数据，屏幕层通过 `TttGet*()` 只读接口取数据。

## 5. 触发器匹配 `StateFindTable()`

按迁移表顺序逐条比较，返回第一个命中的条目：

- 普通条目：`trigger` 与触发值精确相等。
- 范围触发器：条目的 `trigger` 落在 `STATE_RANGE_MIN`～`STATE_RANGE_MAX` 时，令 `index = trigger - STATE_RANGE_MIN`，从 `g_triggerRanges[index]` 取 `min`/`max`，触发值落在区间内即命中。
- `STATE_RANGE_ANY` 也是范围触发器的一种，通常放在表尾作为兜底；因为按表内顺序匹配，放在前面的条目优先。

例程的 `g_triggerRanges` 只显式配置了索引 0、1，其它下标为零初始化 `{0, 0}`。使用某个 `STATE_RANGE_*` 标识前，必须先在对应下标填好区间。

## 6. 状态历史栈

### 6.1 结构

```c
#define STATE_HISTORY_DEPTH 25u

typedef struct StateHistory_t
{
    const StateType_t *states[STATE_HISTORY_DEPTH];
    UI_8 stackTop;   /* 有效条目数；栈顶下标为 stackTop - 1 */
} StateHistory_t;

extern StateHistory_t stateStack;   /* 定义在 MainStat.c */
```

栈顶始终保存“当前状态”。结构体在 `MainStat.h` 中声明，`stateStack` 实例由 `MainStat.c` 定义，仅供引擎内部使用。

### 6.2 保存（`SaveHistory` → `PushHistory`）

迁移完成后，对目标状态执行：

1. `autoFlg` bit0 为 1 的自动迁移状态是过渡态，不入栈。
2. 从栈顶向前扫描：
   - 已存在 → `stackTop = 位置 + 1`，栈顶回退到该状态，其后较新的历史全部丢弃；
   - 不存在 → 压入栈顶，`stackTop++`。
3. 因此同一状态在历史中只保留一份；再次进入某个状态等于把历史回退到它。

栈深度为 25。栈满视为调用方的流程设计问题，引擎不做丢弃处理。

### 6.3 回退（`StateEngineRecovery`）

| 迁移目标 | 处理 |
|---|---|
| `&g_previousState` | `GetPreviousState()`：`PopHistory()` 弹出当前状态（栈内还有更早条目时），再取新的栈顶 |
| `&g_reenterState` | `GetCurrentState()`：直接取当前栈顶，不弹栈（重新进入当前状态） |
| 普通状态指针 | 不做特殊处理，按普通迁移执行 |

`PopHistory()` 在栈内只有一条记录时不再弹出，栈底状态保留，因此首层状态回退会停留在当前状态。栈为空时 `GetCurrentState()` 返回 0，属于调用方未按约定使用（回退前至少应有一条已保存的状态）。

返回特定状态不经过引擎：工程侧把条目的 `nextState` 直接写成目标状态表指针即可。

## 7. 对外接口

```c
void StateEngineInit(const StateType_t *initialState);
void StateEngine(UI_16 keyCode);
void StateEngineRecovery(void);
BOOL StateFindTable(const StateTable_t **pRule, UI_8 *ruleCount, UI_16 *pTrigger);
```

- 引擎侧还定义了两个特殊迁移目标：

```c
extern const StateType_t g_previousState;
extern const StateType_t g_reenterState;
```

- `UI_8 StateGetHistoryTop(void)` 是调试接口，实现在 `MainStat.c`，声明放在工程侧头文件中（例程为 `Action_StateTransition_Sample.h`）。

## 8. 接入真实工程

1. 用工程公共类型头替换 `TypeDefine_Sample.h`，保证 `MainStat.h` 能拿到 `UI_8/UI_16/BOOL`、`TRUE/FALSE/OK/NG/YES/NO/PUBLIC`、`SCREEN_ID_NONE`。
2. `MainStat.c` 通过 `#include "TriggerDefine_Sample.h"` 使用 `STATE_RANGE_MIN/STATE_RANGE_MAX/g_triggerRanges`。实机换成工程的触发器配置头（保持这三个符号，或修改该 include）。
3. 删除 `MainStat.c` 末尾两个空的 `PlatformEnterCritical/PlatformExitCritical` 实现，改为链接显示平台的实际临界区函数。
4. 在项目自己的迁移表头文件中声明 `g_previousState` / `g_reenterState`，供状态表引用。
5. 工程侧定义状态表和回调；返回键、取消键、屏幕 ID、范围触发器区间全部由项目决定。
6. 例程的 `TttRenderCurrentScreen()` 与 `Screen_StateTransition_Sample.c` 只用于主机演示；实机按 `screenId` 调用 LCD/界面库的绘制函数。
7. 不要修改 `StateEngine()` 的 `while (1)` 循环。

## 9. 教学例程：井字棋（带画面跳转和人机对战）

```text
MENU --Enter--> MENU_DECIDE --自动--> X_TURN / QUIT
X_TURN / O_TURN --方向键--> 同一状态（重绘）

双人：X_TURN / O_TURN --Enter--> CHECK --自动--> X_TURN / O_TURN / X_WIN / O_WIN / DRAW
人机：X_TURN --Enter--> CHECK --自动--> COMPUTER_TURN --自动--> CHECK --自动--> X_TURN / X_WIN / O_WIN / DRAW

X_WIN / O_WIN / DRAW --Enter--> RESULT_DECIDE --自动--> X_TURN / MENU
X_TURN / O_TURN / X_WIN / O_WIN / DRAW --Esc--> MENU
```

- `MENU`：主菜单，`↑/↓` 选择“双人对战 / 人机对战（你执 X） / 退出”，`Enter` 确认。
- `X_TURN` / `O_TURN`：方向键移动棋盘光标，`Enter` 落子，`Esc` 返回主菜单。双人模式两人轮流；人机模式玩家执 X，先手。
- `CHECK`：`autoFlg = YES` 的自动迁移状态。入口函数检查三连线、满盘、以及本回合是否成功落子，返回内部触发器；引擎在同一个 `while` 循环内继续迁移，`CHECK` 本身不入栈。
- 位置已被占用：落子函数不修改棋盘并设置提示，`CHECK` 返回同一回合的触发器，因此不会换人。
- `COMPUTER_TURN`：`autoFlg = YES` 的自动迁移状态，只在人机模式出现。入口函数为电脑执 O 选一个空位落子，返回 `TRIG_TTT_COMPUTER_DONE`，引擎继续进入 `CHECK` 判定；该状态不入栈、不渲染。电脑策略为“能赢则赢、能堵则堵，其次中心/角/边”，不保证最优。
- `X_WIN` / `O_WIN` / `DRAW`：`↑/↓` 选择“再来一局 / 返回主菜单”，`Enter` 确认，`Esc` 直接回主菜单。
- `MENU_DECIDE` / `RESULT_DECIDE`：自动迁移状态，把选中项翻译成 `TRIG_TTT_START_2P` / `TRIG_TTT_START_CPU` / `TRIG_TTT_QUIT` / `TRIG_TTT_REPLAY` / `TRIG_TTT_MENU`，自身不入栈。
- `MENU` 作为初始状态在 `StateEngineInit()` 时不会入栈；菜单上的方向键是自迁移，会按普通状态保存进历史。返回主菜单使用直接目标 `&StateTttMenu`，不依赖 `g_previousState`。

**画面绘制**：入口回调只更新数据（棋盘、光标、菜单选中项、提示），不直接画屏。主循环在每次 `StateEngine()` 返回后按 `STATE_CURRENT()->screenId` 调用 `TttRenderCurrentScreen()`。自动迁移状态的 `screenId` 使用 `SCREEN_ID_NONE`，由引擎在内部执行完，不会作为最终画面出现。人机模式下电脑落子也发生在同一次 `StateEngine()` 调用内，画面会直接回到 X 回合或胜负画面。

棋盘空位不再显示 1～9，直接留空；X/O 正常显示，当前光标用 `[ ]`、`[X]`、`[O]` 标出。

**操作键**：

| 按键 | 作用 |
|---|---|
| `↑↓←→` / `WASD` | 移动光标 / 菜单选择 |
| `Enter` / `Space` | 确认 / 落子 |
| `Esc` | 返回主菜单 |
| `Q` | 直接退出程序 |

Windows 控制台用 `_getch()` 读取按键，方向键不需要回车；其它平台按行输入：输入 `w/a/s/d` 后回车移动，直接回车确认，`q` 退出。人机模式中玩家执 X（先手），电脑执 O，电脑走子不需要按键。引擎本身与这些按键无关。

源码文件统一使用 UTF-8。Windows 下 `TttScreenInit()` 会在 `main()` 最开始执行 `chcp 65001`，把控制台代码页切到 UTF-8，否则中文会被控制台按 GBK 解码成乱码。不要为了终端显示把源码另存为 GBK。

本机运行（GCC / MinGW，或 Windows 下用 `zig cc` 代替 `gcc`）：

```sh
gcc -std=c99 -Wall -Wextra -pedantic MainStat.c TriggerDefine_Sample.c Action_StateTransition_Sample.c Screen_StateTransition_Sample.c main.c -o state_demo
./state_demo
```

```powershell
zig cc -std=c99 -Wall -Wextra -pedantic MainStat.c TriggerDefine_Sample.c Action_StateTransition_Sample.c Screen_StateTransition_Sample.c main.c -o state_demo.exe
.\state_demo.exe
```

交叉编译语法检查（不链接，不需要启动文件）：

```sh
arm-none-eabi-gcc -std=c99 -Wall -Wextra -pedantic -c MainStat.c TriggerDefine_Sample.c Action_StateTransition_Sample.c Screen_StateTransition_Sample.c main.c
```

`output.txt` 是各画面渲染结果的文本记录（`cls` 清屏动作不会写入重定向文件）。
