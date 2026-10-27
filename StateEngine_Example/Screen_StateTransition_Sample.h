#ifndef SCREEN_STATE_TRANSITION_SAMPLE_H
#define SCREEN_STATE_TRANSITION_SAMPLE_H

#include "TypeDefine_Sample.h"

/*=========================================================
 * 井字棋屏幕层
 *
 * 与 Action_StateTransition_Sample 对应：
 * - Action 层负责状态表、棋盘数据和迁移动作
 * - Screen 层负责按键读取、清屏和按 scrnId 绘制画面
 *=========================================================*/

/* 画面 ID：主循环在每次引擎调用后按 scrnId 渲染 */
#define SCRN_TTT_MENU  300u
#define SCRN_TTT_XTURN 301u
#define SCRN_TTT_OTURN 302u
#define SCRN_TTT_XWIN  304u
#define SCRN_TTT_OWIN  305u
#define SCRN_TTT_DRAW  306u
#define SCRN_TTT_QUIT  307u

/* 自动迁移状态是过渡态，不对应画面，scrnId 使用 PT_SCRN_DUMMY */

/* 屏幕层按键返回值：不占用 KC_* 触发值 */
#define SCREEN_KEY_NONE ((UI_16)0x0000u)
#define SCREEN_KEY_QUIT ((UI_16)0xFFFFu)

void TttScreenInit(void);
UI_16 TttScreenReadTrigger(void);
void TttRenderCurrentScreen(void);

#endif /* SCREEN_STATE_TRANSITION_SAMPLE_H */
