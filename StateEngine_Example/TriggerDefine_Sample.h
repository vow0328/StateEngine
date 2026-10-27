#ifndef TRIGGER_DEFINE_SAMPLE_H
#define TRIGGER_DEFINE_SAMPLE_H

#include "MainStat.h"

/*=========================================================
 * 教学例程的工程侧配置（示例值，实机按项目修改）
 *
 * 内容不属于迁移引擎通用部分：
 * - 范围触发器标识 MC_TOP～MC_END
 * - 通配触发器 MC_ANYKEY
 * - 模拟特殊按键 KC_*
 * - 范围触发器对照表 TblMcTrig
 *=========================================================*/

/* 范围触发器标识：GetTrigTbl() 会把该区间的 trig 换算成 TblMcTrig 的下标 */
#define MC_TOP    ((UI_16)0xF000u)
#define MC_END    ((UI_16)0xF003u)
#define MC_ANYKEY ((UI_16)(MC_TOP + 2u))

/* 教学例程模拟的特殊按键（井字棋未使用，保留用于展示工程侧键值定义）
 * 注意：KC_SCROLL 与 KC_ENTER 当前取值相同；实机接入时请按项目键值表
 * 保证各按键值唯一。 */
#define KC_SHORT_OK ((UI_16)0x000Au)
#define KC_SHORT_CR ((UI_16)0x000Bu)
#define KC_TAB_OFF  ((UI_16)0x000Cu)
#define KC_SCROLL   ((UI_16)0x000Du)
#define KC_SCRN     ((UI_16)0x000Eu)
#define KC_PNLEN    ((UI_16)0x000Fu)
#define KC_ESC      ((UI_16)0x001Bu) /* 样例：取消/返回键 */

/* 交互界面按键：方向键、确认键 */
#define KC_UP       ((UI_16)0x0010u)
#define KC_DOWN     ((UI_16)0x0011u)
#define KC_LEFT     ((UI_16)0x0012u)
#define KC_RIGHT    ((UI_16)0x0013u)
#define KC_ENTER    ((UI_16)0x000Du)

/* MC_TOP～MC_END 各下标对应的触发值范围 */
extern const MinToMaxTrig_T TblMcTrig[MC_END - MC_TOP + 1u];

#endif /* TRIGGER_DEFINE_SAMPLE_H */
