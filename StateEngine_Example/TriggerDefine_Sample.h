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

/* 示例按键：上、下、确认和取消。 */
#define KEY_ESC      ((UI_16)0x001Bu)
#define KEY_UP       ((UI_16)0x0010u)
#define KEY_DOWN     ((UI_16)0x0011u)
#define KEY_DELETE   ((UI_16)0x0014u)
#define KEY_ENTER    ((UI_16)0x000Du)

/* STATE_RANGE_MIN～STATE_RANGE_MAX 各下标对应的触发值范围 */
extern const TriggerRange_t g_triggerRanges[STATE_RANGE_MAX - STATE_RANGE_MIN + 1u];

#endif /* TRIGGER_DEFINE_SAMPLE_H */
