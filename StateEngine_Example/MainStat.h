#ifndef MAIN_STAT_H
#define MAIN_STAT_H

#include "TypeDefine_Sample.h" /* 工程基础类型与通用标志（实机替换此文件） */

/*=========================================================
 * 1. 数据结构
 *=========================================================*/

/* 范围迁移规则对应的触发值区间，包含两个端点 */
typedef struct TriggerRange_t
{
    UI_16 min;
    UI_16 max;
} TriggerRange_t;

/* 规则回调返回值传给目标状态进入回调；自动迁移时后者返回下一轮触发值 */
typedef UI_16 FUNC(UI_16);

/* 状态类型前置声明 */
typedef struct StateType_t StateType_t;

/* 单条迁移规则 */
typedef struct StateTable_t
{
    UI_16 trigger;               /* 匹配的触发值或范围标识 */
    FUNC *afFuncPtr;             /* 切换状态前执行的规则回调 */
    const StateType_t *nextState; /* 规则匹配后的目标状态 */
} StateTable_t;

/* 状态定义：进入回调、自动迁移标志和该状态的迁移规则 */
struct StateType_t
{
    FUNC *bfFuncPtr;             /* 进入该状态时执行的回调 */
    UI_8 autoFlg : 2;            /* bit0=1：入口返回值驱动自动迁移，且不入历史栈 */
    UI_8 transitionNum : 6;      /* 迁移规则数量，最大 63 */
    const StateTable_t *atTable; /* 当前状态的迁移规则表 */
    UI_16 screenId;              /* 画面 ID */
};

/* 引擎上下文 */
typedef struct StateInfo
{
    UI_16 trigger;            /* 当前触发值 */
    const StateType_t *state; /* 当前状态 */
} StateInfo;

/* 状态迁移历史栈 */
#define STATE_HISTORY_DEPTH 25u
#define STATE_HISTORY_NOT_FOUND ((UI_8)0xFFu) /* 历史中未找到 */

typedef struct StateHistory_t
{
    const StateType_t *states[STATE_HISTORY_DEPTH]; /* 历史状态，自动迁移状态不入栈 */
    UI_8 stackTop;                                 /* 有效条目数；栈顶下标为 stackTop - 1 */
} StateHistory_t;

/*=========================================================
 * 2. 全局实例
 *=========================================================*/
extern StateInfo stateInfo;
extern StateHistory_t stateStack;

/*=========================================================
 * 3. 状态描述宏
 *=========================================================*/
#define SetStateTrigger(t) (stateInfo.trigger = (UI_16)(t))
#define STATE_GET_TRIGGER() ((UI_16)stateInfo.trigger)
#define STATE_CURRENT() (stateInfo.state)
#define STATE_RULES(tbl) (UI_8)(sizeof(tbl) / sizeof((tbl)[0])), (tbl)

/*=========================================================
 * 4. 引擎接口
 *=========================================================*/
void StateEngineInit(const StateType_t *initialState);
void StateEngine(UI_16 keyCode);
void StateEngineRecovery(void);

BOOL StateFindTable(const StateTable_t **pTable, UI_8 *pRuleCount, UI_16 *pTrigger);

#endif /* MAIN_STAT_H */
