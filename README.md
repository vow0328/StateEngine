# MainStat 表驱动状态迁移引擎

`MainStat.c` / `MainStat.h` 是表驱动状态迁移引擎本体：每个状态指向自己的规则数组，输入触发值后，引擎按规则调用处理函数并切换状态。本仓库包含引擎核心、最小移植模板和交互式数据库选择例程。

建议运行数据库选择例程，并对照 [`StateEngine_Example/状态迁移详解.md`](StateEngine_Example/状态迁移详解.md) 单步观察入口检查、页面内操作、删除和自动迁移。

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

- `MainStat.h` 提供默认的 `STATE_RANGE_MIN`、`STATE_RANGE_MAX`、`STATE_RANGE_ANY`；工程可在包含该头文件前重定义。
- 引擎只在 `MainStat.h` 中声明对照表，不提供默认定义：工程侧必须定义 `g_triggerRanges`。如果不使用范围匹配，规则的 `trigger` 不得落在保留的范围标识区间；即使对照表全为 0，范围规则仍会匹配触发值 0。
- 对照表下标为 `trigger - STATE_RANGE_MIN`，表项为 `{min, max}`。

## 接入真实工程

1. 提供 `UI_8`、`UI_16`、`BOOL`、`TRUE`、`FALSE`、`OK`、`NG`、`YES`、`NO`、`PUBLIC`、`SCREEN_ID_NONE`。可以用项目已有的公共类型头替换 `TypeDefine_Sample.h`，并修改 `MainStat.h` 的 include。
2. 范围触发器按上一节配置 `STATE_RANGE_MIN`、`STATE_RANGE_MAX` 并定义 `g_triggerRanges`；不使用范围匹配时也要提供该表，否则链接缺少符号。配置宏必须在编译 `MainStat.c` 的过程中可见，并与定义对照表的编译单元一致，不能只在业务源文件中定义。
3. `MainStat.c` 末尾有两个空的 `PlatformEnterCritical()` / `PlatformExitCritical()`。实机删除这两个空实现，改为链接显示平台的实际临界区函数。
4. 在项目头文件中声明引擎提供的两个特殊迁移目标：

   ```c
   extern const StateType_t PreviousState;   /* 回退到上一状态（出栈） */
   extern const StateType_t g_reenterState;  /* 取当前状态（栈顶，不弹栈） */
   ```

5. 工程侧定义状态表、入口回调、迁移回调、按键值和屏幕 ID。返回特定状态不需要引擎支持：直接把迁移条目的 `nextState` 写成目标状态指针即可。
6. 调试接口 `UI_8 StateGetHistoryTop(void)` 实现在 `MainStat.c`，声明放在工程侧头文件中，按需使用。
7. **不要修改 `StateEngine()` 内部的 `while (1)` 迁移循环。** 它是整个引擎的核心。

## 设计约束

- 使用引擎前确保当前状态非空；参与迁移的规则回调和目标状态进入回调非空。未匹配到规则时引擎直接返回。
- 自动迁移链应保证最终退出，避免无限循环；未匹配到规则也会返回，此时可能停在自动状态。
- `STATE_RULES(tbl)` 只负责把数组拆成“条目数 + 数组指针”，没有其它作用。
- 状态历史栈深度为 25，当前实现没有栈满检查，超过深度会越界写入；接入方必须限制历史长度。
- 自动迁移状态（`autoFlg` bit0 为 1）是过渡态，不入栈。
- 初始化不会将初始状态入栈，自动迁移状态也不入栈，因此栈顶不总是当前状态。使用特殊回退目标前必须保证栈非空；为空时会得到空指针，后续解引用不安全。
- 引擎不负责画面绘制；画面由工程侧根据 `STATE_CURRENT()->screenId` 决定。

## 完整例程

数据库例程演示自动入口检查、单独的选择页初始化、页面内自迁移、历史回退、删除工作状态和自动结果检查；数据仅保存在内存中。编译与运行方式见 [`StateEngine_Example/README.md`](StateEngine_Example/README.md)。
