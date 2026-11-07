#ifndef TRIGGER_DEFINE_SAMPLE_H
#define TRIGGER_DEFINE_SAMPLE_H

#include "MainStat.h"

/*=========================================================
 * 教学例程的工程侧配置（示例值，实机按项目修改）
 *
 * 内容不属于迁移引擎通用部分：
 * - 范围触发器标识 STATE_RANGE_MIN～STATE_RANGE_MAX
 * - 迁移表的无条件匹配标识 STATE_ANY
 * - 模拟特殊按键 KEY_*
 * - 范围触发器对照表 TriggerRanges
 *=========================================================*/

/* 范围触发器标识：StateFindTable() 会把迁移表项中该区间内的 trigger 换算成
 * TriggerRanges 的下标。若工程已定义这些宏，则沿用工程配置。 */
#ifndef STATE_RANGE_MIN
#define STATE_RANGE_MIN ((UI_16)0xF000u)
#endif
#ifndef STATE_RANGE_MAX
#define STATE_RANGE_MAX ((UI_16)0xF006u)
#endif
#ifndef STATE_ANY
#define STATE_ANY ((UI_16)0xFFFFu)
#endif

/* 迁移表使用的按键触发值：方向、确认、取消、删除及首页等操作。 */
#define KEY_ESC      ((UI_16)0x001Bu)
#define KEY_UP       ((UI_16)0x0010u)
#define KEY_DOWN     ((UI_16)0x0011u)
#define KEY_DELETE   ((UI_16)0x0014u)
#define KEY_ERROR    ((UI_16)0x0015u)
#define KEY_HOME     ((UI_16)0x0016u)
#define KEY_ENTER    ((UI_16)0x000Du)
#define KEY_BACKSPACE ((UI_16)0x0008u)
#define KEY_LEFT     ((UI_16)0x0018u)
#define KEY_RIGHT    ((UI_16)0x0019u)

#define RANGE_DIGIT   ((UI_16)(STATE_RANGE_MIN + 0u))
#define RANGE_UPPER   ((UI_16)(STATE_RANGE_MIN + 1u))
#define RANGE_LOWER   ((UI_16)(STATE_RANGE_MIN + 2u))
#define RANGE_SYMBOL1 ((UI_16)(STATE_RANGE_MIN + 3u))
#define RANGE_SYMBOL2 ((UI_16)(STATE_RANGE_MIN + 4u))
#define RANGE_SYMBOL3 ((UI_16)(STATE_RANGE_MIN + 5u))
#define RANGE_SYMBOL4 ((UI_16)(STATE_RANGE_MIN + 6u))

/* STATE_RANGE_MIN～STATE_RANGE_MAX 各下标对应的触发值范围 */
extern const TriggerRange_t TriggerRanges[STATE_RANGE_MAX - STATE_RANGE_MIN + 1u];

#endif /* TRIGGER_DEFINE_SAMPLE_H */
