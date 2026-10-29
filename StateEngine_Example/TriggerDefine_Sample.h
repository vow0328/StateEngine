#ifndef TRIGGER_DEFINE_SAMPLE_H
#define TRIGGER_DEFINE_SAMPLE_H

#include "MainStat.h"

/*=========================================================
 * 教学例程的工程侧配置（示例值，实机按项目修改）
 *
 * 内容不属于迁移引擎通用部分：
 * - 范围触发器标识 STATE_RANGE_MIN～STATE_RANGE_MAX
 * - 通配触发器 STATE_RANGE_ANY
 * - 模拟特殊按键 KEY_*
 * - 范围触发器对照表 g_triggerRanges
 *=========================================================*/

/* 范围触发器标识：StateFindTable() 会把该区间的 trigger 换算成
 * g_triggerRanges 的下标。若工程已定义这些宏，则沿用工程配置。 */
#ifndef STATE_RANGE_MIN
#define STATE_RANGE_MIN ((UI_16)0xF000u)
#endif
#ifndef STATE_RANGE_MAX
#define STATE_RANGE_MAX ((UI_16)0xF003u)
#endif
#ifndef STATE_RANGE_ANY
#define STATE_RANGE_ANY ((UI_16)(STATE_RANGE_MIN + 2u))
#endif

/* 教学例程模拟的特殊按键（井字棋未使用，保留用于展示工程侧键值定义）
 * 注意：KEY_SCROLL 与 KEY_ENTER 当前取值相同；实机接入时请按项目键值表
 * 保证各按键值唯一。 */
#define KEY_SHORT_OK ((UI_16)0x000Au)
#define KEY_SHORT_CR ((UI_16)0x000Bu)
#define KEY_TAB_OFF  ((UI_16)0x000Cu)
#define KEY_SCROLL   ((UI_16)0x000Du)
#define KEY_SCRN     ((UI_16)0x000Eu)
#define KEY_PNLEN    ((UI_16)0x000Fu)
#define KEY_ESC      ((UI_16)0x001Bu) /* 样例：取消/返回键 */

/* 交互界面按键：方向键、确认键 */
#define KEY_UP       ((UI_16)0x0010u)
#define KEY_DOWN     ((UI_16)0x0011u)
#define KEY_LEFT     ((UI_16)0x0012u)
#define KEY_RIGHT    ((UI_16)0x0013u)
#define KEY_ENTER    ((UI_16)0x000Du)

/* STATE_RANGE_MIN～STATE_RANGE_MAX 各下标对应的触发值范围 */
extern const TriggerRange_t g_triggerRanges[STATE_RANGE_MAX - STATE_RANGE_MIN + 1u];

#endif /* TRIGGER_DEFINE_SAMPLE_H */
