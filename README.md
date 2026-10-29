# MainStat 表驱动状态迁移引擎

`MainStat.c` / `MainStat.h` 是表驱动状态迁移引擎本体。本仓库包含引擎核心、最小移植模板和完整交互式井字棋例程；例程源码在 `StateEngine_Example/` 目录，不压缩入库。

## 文件

| 文件 | 说明 |
|---|---|
| `MainStat.c` / `MainStat.h` | **引擎本体**：迁移循环、触发器匹配、状态历史栈、特殊迁移目标、临界区占位。 |
| `TypeDefine_Sample.h` | 最小类型与通用标志模板：`UI_8` / `UI_16` / `BOOL`、`TRUE/FALSE/OK/NG/YES/NO/PUBLIC`、`SCREEN_ID_NONE`。实机替换为项目公共类型头。 |
| [`StateEngine_Example/`](StateEngine_Example/) | 完整交互式井字棋例程源码，包含 Action/Screen 分层、双人/人机对战。 |

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
- `MainStat.c` 提供一个全 0 的弱符号 `g_triggerRanges`，默认情况下范围触发器不会命中任何值，普通触发器不受影响。
- 需要范围触发器时：
  - GCC / Clang：直接定义同名强符号 `g_triggerRanges`，覆盖弱定义；
  - 其它编译器：定义 `STATE_EXTERNAL_TRIGGER_TABLE`，并自行提供 `g_triggerRanges`。
- 对照表下标为 `trigger - STATE_RANGE_MIN`，表项为 `{min, max}`。

## 接入真实工程

1. 提供 `UI_8`、`UI_16`、`BOOL`、`TRUE`、`FALSE`、`OK`、`NG`、`YES`、`NO`、`PUBLIC`、`SCREEN_ID_NONE`。可以用项目已有的公共类型头替换 `TypeDefine_Sample.h`，并修改 `MainStat.h` 的 include。
2. 范围触发器按上一节配置 `STATE_RANGE_MIN`、`STATE_RANGE_MAX` 和 `g_triggerRanges`；不使用范围触发器时保持默认全 0 表即可。
3. `MainStat.c` 末尾有两个空的 `PlatformEnterCritical()` / `PlatformExitCritical()`。实机删除这两个空实现，改为链接显示平台的实际临界区函数。
4. 在项目头文件中声明引擎提供的两个特殊迁移目标：

   ```c
   extern const StateType_t g_previousState;   /* 回退到上一状态（出栈） */
   extern const StateType_t g_reenterState;  /* 取当前状态（栈顶，不弹栈） */
   ```

5. 工程侧定义状态表、入口回调、迁移回调、按键值和屏幕 ID。返回特定状态不需要引擎支持：直接把迁移条目的 `nextState` 写成目标状态指针即可。
6. 调试接口 `UI_8 StateGetHistoryTop(void)` 实现在 `MainStat.c`，声明放在工程侧头文件中，按需使用。
7. **不要修改 `StateEngine()` 内部的 `while (1)` 迁移循环。** 它是整个引擎的核心。

## 设计约束

- 迁移表数据非空；自动迁移标志最终一定会变成 `NO`。
- `STATE_RULES(tbl)` 只负责把数组拆成“条目数 + 数组指针”，没有其它作用。
- 状态历史栈深度为 25；栈满不丢弃，视为调用方的流程设计问题。
- 自动迁移状态（`autoFlg` bit0 为 1）是过渡态，不入栈。
- 状态栈保存的是“当前状态”；回退时先弹出当前状态，再取新的栈顶。
- 引擎不负责画面绘制；画面由工程侧根据 `STATE_CURRENT()->screenId` 决定。

## 完整例程

交互式井字棋例程（双人对战 / 人机对战）演示菜单跳转、自动迁移、状态回退、画面渲染和状态栈行为。完整源码在仓库根目录的 [`StateEngine_Example/`](StateEngine_Example/) 目录中，编译与运行方式见该目录的 README。需要单文件分发包时，可从该目录生成 zip 挂到 GitHub Release，不把压缩包提交进 Git。
