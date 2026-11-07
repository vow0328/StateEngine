# 数据库选择状态迁移例程

本例使用一组**内存中的模拟名称和表格数据**，不连接、不修改任何真实数据库或文件。它重点展示：每次切换画面都先经过 `autoFlg = YES` 的中间过渡状态表，再进入 `autoFlg = NO` 的画面状态表；同一画面内移动和输入直接自迁移，不重复执行跨画面初始化。

入口文件为 `main.c`，引擎为本目录的 `MainStat.c/.h` 副本，根目录为引擎核心。状态表、迁移表数组和业务数据位于 `Action_StateTransition_Sample.c`；输入和显示位于 `Screen_StateTransition_Sample.c`。逐步执行和指针图见 [状态迁移详解](状态迁移详解.md)。

本项目中，`StateType_t` 定义**状态表**，`StateTable_t` 定义**迁移表**。例如 `StateDatabaseSelect` 是状态表，其 `transTbl` 指向 `DatabaseSelectRules` 迁移表数组，`transNum` 是数组条目数。数组中的每个迁移表项保存迁移条件、后处理函数和目标状态表指针；状态表本身保存前处理函数、自动迁移标志和画面 ID 等信息。

## 状态分层

```text
当前画面状态表（NO）
  ├─ 页内移动或输入 → 后处理 → 当前画面状态表（NO）
  └─ 切换画面 → 入口或回退状态表（YES，SCREEN_ID_NONE）
                         └─ 检查、初始化或解析历史 → 目标画面状态表（NO）
```

| 画面切换 | 自动过渡状态表（`YES`） | 最终画面状态表（`NO`） |
|---|---|---|
| 启动准备、空数据页或错误页返回首页 | `StateHomeEntry` | `StateHome` |
| 首页进入数据库选择页 | `StateDatabaseEntry` | 正常进入 `StateDatabaseSelect`，无数据或故障进入相应提示页 |
| 选择页打开数据库内容 | `StateContentEntry` | `StateDatabaseContent` |
| 内容页进入单元格编辑 | `StateEditEntry` | `StateDatabaseEdit` |
| 选择页打开删除确认 | `StateConfirmEntry` | `StateDeleteConfirm` |
| 确认删除后进入工作画面 | `StateWorkingEntry` | `StateDeleteWorking` |
| 删除执行完成 | `StateDeleteCheck`，有剩余时再经过 `StateDeleteResume` | `StateDatabaseSelect` 或 `StateDatabaseEmpty` |
| 选择页、内容页返回，编辑保存或取消，取消删除 | `StateBackEntry` → `PreviousState` | 历史中的上一画面状态表 |
| 内容页返回指定首页 | `StateHomeBackEntry` → `FormEntryState` | 历史中定位到的 `StateHome` |

`StateContentEntry`、`StateConfirmEntry`、`StateWorkingEntry` 检查数据库是否存在及所选条目是否有效；`StateEditEntry` 还检查行列边界。它们的迁移表都覆盖正常、无数据和错误返回值，分别进入目标画面、空数据页或错误页。`OpenDatabase` 只在内容页入口的正常分支重置光标，`BeginEdit` 只在编辑页入口的正常分支备份单元格。

`YES` 表示**过渡状态表的前处理执行后**，使用其返回值继续查询迁移表，在同一次 `StateEngine()` 调用内到达画面状态表；不需要第二次按键。过渡状态表的画面 ID 是 `SCREEN_ID_NONE`，不会显示，也不进入历史栈。`NO` 表示前处理执行后返回调用方，供主循环渲染并等待事件。`StateDeleteWorking` 也是画面状态表，先由 `StateWorkingEntry` 自动切入，显示后再等待执行事件，不能把工作画面本身设为自动过渡态而跳过显示。

本例的 `DatabaseContext` 聚合列表、选择项和模拟故障标志。迁移表项的 `InitDatabaseSelect` 后处理将选择项复位；`DummyFunc` 是无操作占位函数，原样返回输入值，供无需额外处理的状态表前处理或迁移表项后处理使用。画面由主循环根据当前状态表的画面 ID 渲染。

## 编译运行

在 `StateEngine_Example` 目录，使用本机 GCC 或 MinGW：

```sh
gcc -std=c99 -Wall -Wextra -pedantic MainStat.c TriggerDefine_Sample.c Action_StateTransition_Sample.c Screen_StateTransition_Sample.c main.c -o state_demo
./state_demo
```

Windows 下也可使用 `zig cc` 代替 `gcc`，输出为 `state_demo.exe` 并运行 `.\state_demo.exe`。在 Windows 交互式控制台中按键立即生效，无需每次按 Enter；屏幕会刷新并显示当前选择、操作提示和模拟删除结果。

| 操作 | Windows 交互式控制台 |
|---|---|
| 进入选择页、确认删除 | `Enter` |
| 打开选中的数据库内容 | `Enter` |
| 内容页上下左右选择单元格 | 方向键 `↑` / `↓` / `←` / `→` 或 `W` / `S` / `A` / `D` |
| 内容页修改选中单元格 | `Enter` |
| 编辑时保存 / 取消 | `Enter` / `Esc` |
| 首页切换入口检查模拟故障 | `E` |
| 上移 / 下移 | 方向键 `↑` / `↓` 或 `W` / `S` |
| 打开删除确认 | `D` 或 `Delete` |
| 取消或返回 | `Esc` 或 `C` |
| 内容页直接返回首页 | `H` |
| 退出 | `Q` |

非 Windows 平台和标准输入重定向使用行输入：输入 `w`、`a`、`s`、`d`、`e`、`h`、`c` 或 `q` 后按 Enter，确认、打开或修改则输入空行；输入结束也会退出。`D` 在选择页用于删除，在内容页用于向右移动。重定向输出时不清屏、不输出颜色控制字符，便于查看日志或脚本验证。Windows 控制台正常退出时会恢复原输出代码页和终端模式。

实际 STM32 编译需要平台启动、链接脚本和输入输出适配。仅检查语法时可使用 `arm-none-eabi-gcc -std=c99 -Wall -Wextra -pedantic -fsyntax-only` 加上同一组源文件；交叉编译器生成的程序不能直接当作 PC 程序运行。

## 建议观察

1. 启动时准备事件先经过 `StateHomeEntry` 自动进入首页；从首页按 Enter，再经过 `StateDatabaseEntry` 检查和 `InitDatabaseSelect` 初始化，进入列表。
2. 按下方向键或 `S` / `W`：选择项改变，命中的迁移表项直接指向选择页状态表，不经过入口检查或重新初始化。可在 `CheckDatabaseSelect` 和 `InitDatabaseSelect` 设置断点验证。
3. 按 `D`：经 `StateConfirmEntry` 检查后显示删除确认；再按 Enter，经 `StateWorkingEntry` 检查后显示工作画面。主循环随后发出执行事件，结果检查后回到列表或空数据页。
4. 重复删除直到没有数据：自动检查进入空数据页面；输入 `c` 返回首页，再按 Enter，入口检查直接进入空数据页面。
5. 选择页输入 `c`：先进入 `StateBackEntry`，再自动通过 `PreviousState` 返回首页。启动时 `main.c` 发送准备事件，经 `StateHomeEntry` 进入首页并保存历史；`StateEngineInit()` 自身不会把初始状态入栈。
6. 首页按 `E` 开启模拟故障，再按 Enter：入口状态表的前处理返回 `EVENT_DB_ERROR`，自动命中错误条件的迁移表项并进入错误页；按 `C` 返回首页，再按 `E` 关闭故障即可正常进入。
7. 在选择页选中数据库并按 Enter，经 `StateContentEntry` 检查并初始化光标后查看内容。按 Enter 经 `StateEditEntry` 备份单元格后编辑；保存或取消经 `StateBackEntry` 返回内容页，保持光标位置。返回选择页也经过 `StateBackEntry`，保持数据库选择项。
8. 内容页按 `H`：先进入 `StateHomeBackEntry`，其迁移表项后处理定位历史首页并截断后续历史，再通过 `FormEntryState` 自动进入首页。

`StateBackEntry` 和 `StateHomeBackEntry` 都是自动回退过渡状态表，不会调用前进入口的初始化函数，因此返回时不会重置选择项、内容页光标或编辑备份。`FormEntryState` 只是取历史栈顶指针的特殊占位，首页定位由 `StateHomeBackEntry` 迁移表项的后处理完成。若历史中找不到首页，后处理保留原栈，最终重新进入原栈顶指向的状态。初始内容为模拟记录，删除操作仅从列表移除条目。

内容页显示两列模拟数据（编号、内容），用方向键或 `W/A/S/D` 选择单元格，再按 `Enter` 修改；支持 ANSI 的终端用反色高亮，其他输出用方括号标记。编辑时，Windows 控制台可直接输入字符，`Backspace` 删除末尾，`Enter` 保存，`Esc` 取消并恢复该单元格的原值。行输入模式下，输入一行文字并回车追加；`:back` 删除末尾，`:save` 或空行保存，`:cancel` 取消。编辑模式中的字母不会触发普通页面的快捷键；数据仅保存在内存中，每个单元格最多 23 字节。编辑迁移表分别匹配数字、字母及 ASCII 空格和常用符号，符号按不连续区间拆为多个迁移表项。

删除后仍有数据时，`StateDeleteResume` 是自动迁移状态表，关联的迁移表只有一个表项，使用 `STATE_ANY` 无条件进入选择页并初始化，不依赖前处理返回的具体触发值。迁移表项按数组顺序匹配，通配兜底表项应放在其他表项之后。

## 接入注意

- `StateEngineInit()` 不会自动执行后续迁移，也不保存初始状态；使用历史回退前须确认历史非空且包含预期来源。
- 自动迁移状态表的前处理返回值应能命中其迁移表项，包括业务错误条件；未命中时当前指针仍可能指向该状态表，循环匹配则可能不返回。
- 当前历史栈容量为 25，未实现栈满检查；空状态、空回调或空历史回退也可能引发非法访问。
- `PlatformEnterCritical()` / `PlatformExitCritical()` 在例程中为空；实机须按平台实现。引擎仅保护回调调用，并不保证整个迁移原子化。
- 范围触发器的标识和 `TriggerRanges` 需要在各编译单元中一致；本例用迁移表的范围条件匹配编辑输入。未初始化范围 `{0, 0}` 仍会匹配触发值 0；`STATE_ANY` 不查询范围表。
