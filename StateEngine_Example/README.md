# 数据库选择状态迁移例程

本例使用一组**内存中的模拟名称**，不连接、不修改任何真实数据库或文件。它重点展示：进入页面时自动检查并初始化，页面内移动时不重复初始化，删除时先显示工作状态，再自动检查结果。

入口文件为 `main.c`，引擎为本目录的 `MainStat.c/.h` 副本，根目录为引擎核心。状态表和业务数据位于 `Action_StateTransition_Sample.c`；输入和显示位于 `Screen_StateTransition_Sample.c`。逐步执行和指针图见 [状态迁移详解](状态迁移详解.md)。

## 状态分层

```text
HOME (NO)
  └─ Enter → DatabaseEntry (YES：检查数据库)
                 ├─ 有数据 → InitDatabaseSelect → DatabaseSelect (NO)
                 └─ 无数据 ─────────────────────→ DatabaseEmpty (NO)

DatabaseSelect (NO)
  ├─ 上/下 → 移动选项 → DatabaseSelect (自迁移，仅重新进入画面)
  ├─ 取消 → PreviousState → HOME
  └─ 删除 → DeleteConfirm (NO)
               ├─ 取消 → DatabaseSelect
               └─ 确认 → DeleteWorking (NO：先显示工作画面)
                            └─ 外部执行事件 → 删除 → DeleteCheck (YES)
                                 ├─ 有剩余 → InitDatabaseSelect → DatabaseSelect
                                 └─ 已删空 ─────────────────────→ DatabaseEmpty
```

`YES` 表示**目标状态入口回调执行后**，使用其返回的触发值继续查该状态的规则表；`NO` 表示进入回调执行后返回调用方，不是禁止迁移。`DatabaseEntry` 和 `DeleteCheck` 是自动过渡状态，不入历史栈；`DeleteWorking` 是普通状态，先让主循环渲染，再由主循环模拟产生执行事件。

本例的 `DatabaseContext` 聚合列表、选择项和计数器。`InitDatabaseSelect` 的计数展示相对重的页面初始化；`DisplayScreen` 的计数展示普通状态的进入回调，实际渲染由主循环完成。计数器是教学用可观测指标，不代表实际耗时。

## 编译运行

在 `StateEngine_Example` 目录，使用本机 GCC 或 MinGW：

```sh
gcc -std=c99 -Wall -Wextra -pedantic MainStat.c TriggerDefine_Sample.c Action_StateTransition_Sample.c Screen_StateTransition_Sample.c main.c -o state_demo
./state_demo
```

Windows 下也可使用 `zig cc` 代替 `gcc`，输出为 `state_demo.exe` 并运行 `.\state_demo.exe`。在 Windows 交互式控制台中按键立即生效，无需每次按 Enter；屏幕会刷新并显示当前选择、操作提示、模拟删除结果和状态计数。

| 操作 | Windows 交互式控制台 |
|---|---|
| 进入选择页、确认删除 | `Enter` |
| 上移 / 下移 | 方向键 `↑` / `↓` 或 `W` / `S` |
| 打开删除确认 | `D` 或 `Delete` |
| 取消或返回 | `Esc` 或 `C` |
| 退出 | `Q` |

非 Windows 平台和标准输入重定向使用行输入：输入 `w`、`s`、`d`、`c` 或 `q` 后按 Enter，确认则输入空行；输入结束也会退出。重定向输出时不清屏、不输出颜色控制字符，便于查看日志或脚本验证。Windows 控制台正常退出时会恢复原输出代码页和终端模式。

实际 STM32 编译需要平台启动、链接脚本和输入输出适配。仅检查语法时可使用 `arm-none-eabi-gcc -std=c99 -Wall -Wextra -pedantic -fsyntax-only` 加上同一组源文件；交叉编译器生成的程序不能直接当作 PC 程序运行。

## 建议观察

1. 从首页直接按 Enter：进入列表时 `INIT` 从 0 变为 1。
2. 按下方向键或 `S` / `W`：选择项改变、`DISPLAY` 增加，`INIT` 保持不变，说明页内操作没有经过入口初始化。
3. 按 `D`、再按 Enter：先显示模拟工作画面，主循环随后发出执行事件；如果还有数据，结果检查后回到列表，`INIT` 加 1。
4. 重复删除直到没有数据：自动检查进入空数据页面；输入 `c` 返回首页，再按 Enter，入口检查直接进入空数据页面。
5. 选择页输入 `c`：使用 `PreviousState` 返回首页。启动时 `main.c` 特意发送首页自迁移事件以保存首页；`StateEngineInit()` 自身不会把初始状态入栈。

## 接入注意

- `StateEngineInit()` 不会自动执行后续迁移，也不保存初始状态；使用历史回退前须确认历史非空且包含预期来源。
- 自动状态的入口返回值必须匹配其迁移表；未匹配会停在该状态，循环匹配则可能不返回。
- 当前历史栈容量为 25，未实现栈满检查；空状态、空回调或空历史回退也可能引发非法访问。
- `PlatformEnterCritical()` / `PlatformExitCritical()` 在例程中为空；实机须按平台实现。引擎仅保护回调调用，并不保证整个迁移原子化。
- 范围触发器的标识和 `g_triggerRanges` 需要在各编译单元中一致；本例未使用范围规则。未初始化范围 `{0, 0}` 仍会匹配触发值 0。
