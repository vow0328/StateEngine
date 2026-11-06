# 数据库选择状态迁移例程

本例使用一组**内存中的模拟名称**，不连接、不修改任何真实数据库或文件。它重点展示：进入页面时自动检查并初始化，页面内移动时不重复初始化，删除时先显示工作状态，再自动检查结果。

入口文件为 `main.c`，引擎为本目录的 `MainStat.c/.h` 副本，根目录为引擎核心。状态表和业务数据位于 `Action_StateTransition_Sample.c`；输入和显示位于 `Screen_StateTransition_Sample.c`。逐步执行和指针图见 [状态迁移详解](状态迁移详解.md)。

## 状态分层

```text
HOME (NO)
  └─ Enter → DatabaseEntry (YES：检查数据库)
                 ├─ 模拟故障 ─────────────────────→ DatabaseError (NO)
                 ├─ 有数据 → InitDatabaseSelect → DatabaseSelect (NO)
                 └─ 无数据 ─────────────────────→ DatabaseEmpty (NO)

DatabaseSelect (NO)
  ├─ 上/下 → 移动选项 → DatabaseSelect (自迁移，仅重新进入画面)
  ├─ Enter → DatabaseContent (NO：查看选中数据库的模拟内容)
  │            ├─ 方向键/WASD → 选择单元格
  │            ├─ Enter → DatabaseEdit (NO：范围规则匹配输入字符)
  │            │        └─ 保存/取消 → PreviousState → DatabaseContent
  │            ├─ 取消 → PreviousState → DatabaseSelect
  │            └─ H → 定位历史中的 HOME → FormEntryState → HOME
  ├─ 取消 → PreviousState → HOME
  └─ 删除 → DeleteConfirm (NO)
               ├─ 取消 → DatabaseSelect
               └─ 确认 → DeleteWorking (NO：先显示工作画面)
                            └─ 外部执行事件 → 删除 → DeleteCheck (YES)
                                 ├─ 有剩余 → DeleteResume (YES：STATE_ANY) → InitDatabaseSelect → DatabaseSelect
                                 └─ 已删空 ─────────────────────→ DatabaseEmpty
```

`YES` 表示**目标状态前处理函数执行后**，使用其返回的触发值继续查该状态的规则表；`NO` 表示前处理函数执行后返回调用方，不是禁止迁移。`DatabaseEntry` 和 `DeleteCheck` 是自动过渡状态，不入历史栈；`DeleteWorking` 是普通状态，先让主循环渲染，再由主循环模拟产生执行事件。

本例的 `DatabaseContext` 聚合列表、选择项和模拟故障标志。`InitDatabaseSelect` 后处理函数将选择项复位；`DummyFunc` 是无操作占位函数，原样返回输入值，供无需额外处理的前处理或后处理使用。画面由主循环根据当前状态渲染。

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

1. 从首页按 Enter：经过自动入口检查和 `InitDatabaseSelect`，进入列表。
2. 按下方向键或 `S` / `W`：选择项改变，规则直接回到选择状态，不经过入口检查或重新初始化。可在 `CheckDatabaseSelect` 和 `InitDatabaseSelect` 设置断点验证。
3. 按 `D`、再按 Enter：先显示模拟工作画面，主循环随后发出执行事件；如果还有数据，结果检查后重新初始化并回到列表。
4. 重复删除直到没有数据：自动检查进入空数据页面；输入 `c` 返回首页，再按 Enter，入口检查直接进入空数据页面。
5. 选择页输入 `c`：使用 `PreviousState` 返回首页。启动时 `main.c` 特意发送首页自迁移事件以保存首页；`StateEngineInit()` 自身不会把初始状态入栈。
6. 首页按 `E` 开启模拟故障，再按 Enter：入口前处理返回 `EVENT_DB_ERROR`，自动匹配错误规则并进入错误页；按 `C` 返回首页，再按 `E` 关闭故障即可正常进入。
7. 在选择页选中数据库并按 Enter 查看模拟内容。按 `C` 使用 `PreviousState` 返回选择页；再次打开后按 `H`，后处理先在历史栈中定位首页并截断其后的历史，再通过 `FormEntryState` 取栈顶进入首页。

内容页的 `H` 规则演示返回特定状态：`FormEntryState` 本身只取历史栈顶，并不会搜索首页。例程启动时已将首页入栈；如果历史中找不到首页，后处理保留原栈，`FormEntryState` 将重新进入原栈顶，而不是自动跳转首页。初始内容为模拟记录，删除操作仅从列表移除条目。

内容页显示两列模拟数据（编号、内容），用方向键或 `W/A/S/D` 选择单元格，再按 `Enter` 修改；支持 ANSI 的终端用反色高亮，其他输出用方括号标记。编辑时，Windows 控制台可直接输入字符，`Backspace` 删除末尾，`Enter` 保存，`Esc` 取消并恢复该单元格的原值。行输入模式下，输入一行文字并回车追加；`:back` 删除末尾，`:save` 或空行保存，`:cancel` 取消。编辑模式中的字母不会触发普通页面的快捷键；数据仅保存在内存中，每个单元格最多 23 字节。输入规则分别处理数字、字母及 ASCII 空格和常用符号，符号按不连续区间拆为多条规则。

删除后仍有数据时，`StateDeleteResume` 是自动过渡状态，它的唯一规则使用 `STATE_ANY` 无条件进入选择页并初始化；它不依赖前处理返回的具体触发值。规则按表顺序匹配，使用通配兜底时应放在其他规则之后。

## 接入注意

- `StateEngineInit()` 不会自动执行后续迁移，也不保存初始状态；使用历史回退前须确认历史非空且包含预期来源。
- 自动状态的前处理返回值必须匹配其迁移表，包括业务错误条件；未匹配会停在该状态，循环匹配则可能不返回。
- 当前历史栈容量为 25，未实现栈满检查；空状态、空回调或空历史回退也可能引发非法访问。
- `PlatformEnterCritical()` / `PlatformExitCritical()` 在例程中为空；实机须按平台实现。引擎仅保护回调调用，并不保证整个迁移原子化。
- 范围触发器的标识和 `TriggerRanges` 需要在各编译单元中一致；本例用范围规则匹配编辑输入。未初始化范围 `{0, 0}` 仍会匹配触发值 0；`STATE_ANY` 不查询范围表。
