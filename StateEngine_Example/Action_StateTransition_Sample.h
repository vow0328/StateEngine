#ifndef ACTION_STATE_TRANSITION_SAMPLE_H
#define ACTION_STATE_TRANSITION_SAMPLE_H

#include "MainStat.h"
#include "TriggerDefine_Sample.h"

/*=========================================================
 * 井字棋 Action 层
 *
 * 职责：
 * - 持有棋盘、菜单/结果选择、对战模式等业务数据
 * - 定义状态迁移表和 StateType_t 状态对象
 * - 通过 TttGet*() 向 Screen 层提供只读数据
 *
 * 状态流转示意见 Action_StateTransition_Sample.c 顶部注释。
 *=========================================================*/

/* 自动迁移状态内部触发器（由状态进入回调返回，不来自按键） */
#define TRIG_TTT_TO_X          ((UI_16)0x0A01u)
#define TRIG_TTT_TO_O          ((UI_16)0x0A02u)
#define TRIG_TTT_X_WIN         ((UI_16)0x0A03u)
#define TRIG_TTT_O_WIN         ((UI_16)0x0A04u)
#define TRIG_TTT_DRAW          ((UI_16)0x0A05u)
#define TRIG_TTT_TO_CPU        ((UI_16)0x0A06u)
#define TRIG_TTT_COMPUTER_DONE ((UI_16)0x0A07u)

/* 菜单/结果确认后使用的内部触发器 */
#define TRIG_TTT_START_2P      ((UI_16)0x0B01u)
#define TRIG_TTT_START_CPU     ((UI_16)0x0B02u)
#define TRIG_TTT_QUIT          ((UI_16)0x0B03u)
#define TRIG_TTT_REPLAY        ((UI_16)0x0B04u)
#define TRIG_TTT_MENU          ((UI_16)0x0B05u)

/* 菜单、结果画面的选择项 */
#define TTT_MENU_2P   0u
#define TTT_MENU_CPU  1u
#define TTT_MENU_QUIT 2u

#define TTT_RESULT_REPLAY 0u
#define TTT_RESULT_MENU   1u

/* 对战模式：双人，或玩家执 X / 电脑执 O */
#define TTT_MODE_2P  0u
#define TTT_MODE_CPU 1u

/* 引擎提供的特殊迁移目标（定义在 MainStat.c） */
extern const StateType_t g_previousState; /* 回退到上一状态（出栈） */
extern const StateType_t g_reenterState;  /* 取当前状态（栈顶，不弹栈） */

/* 调试用：当前状态历史深度（引擎实现） */
UI_8 StateGetHistoryTop(void);

/* 井字棋状态对象（与 Action_StateTransition_Sample.c 中的状态表一一对应） */
extern const StateType_t StateTttMenu;
extern const StateType_t StateTttMenuDecide;
extern const StateType_t StateTttXTurn;
extern const StateType_t StateTttOTurn;
extern const StateType_t StateTttCheck;
extern const StateType_t StateTttComputerTurn;
extern const StateType_t StateTttXWin;
extern const StateType_t StateTttOWin;
extern const StateType_t StateTttDraw;
extern const StateType_t StateTttResultDecide;
extern const StateType_t StateTttQuit;

/* 屏幕层读取棋盘数据的只读接口（数据由 Action 层持有） */
char TttGetCell(UI_8 idx);
UI_8 TttGetCursor(void);
char TttGetCurrentMover(void);
UI_8 TttGetMenuSel(void);
UI_8 TttGetResultSel(void);
UI_8 TttGetMode(void);
const char *TttGetNotice(void);

#endif /* ACTION_STATE_TRANSITION_SAMPLE_H */
