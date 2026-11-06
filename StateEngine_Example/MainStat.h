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

/* 后处理函数返回值传给目标状态的前处理函数；自动迁移时前处理返回下一轮触发值 */
typedef UI_16 FUNC(UI_16);

/* 状态类型前置声明 */
typedef struct StateType_t StateType_t;

/* 单条迁移规则 */
typedef struct StateTable_t
{
    UI_16 trigger;                /* 匹配的触发值或范围标识 */
    FUNC *onExit;                 /* 命中规则后、切换状态前执行的后处理函数 */
    const StateType_t *nextState; /* 规则匹配后的目标状态 */
} StateTable_t;

/* 状态定义：前处理函数、自动迁移标志和该状态的迁移规则 */
struct StateType_t
{
    FUNC *onEnter;             /* 进入该状态时执行的前处理函数 */
    UI_8 autoRun : 2;          /* bit0=1：前处理返回值驱动自动迁移，且不入历史栈 */
    UI_8 ruleCount : 6;        /* 迁移规则数量，最大 63 */
    const StateTable_t *rules; /* 当前状态的迁移规则表 */
    UI_16 screenId;            /* 画面 ID */
};

/* 引擎上下文 */
typedef struct StateInfo
{
    UI_16 trigger;            /* 当前触发值 */
    const StateType_t *state; /* 当前状态 */
} StateInfo;

/* 状态迁移历史栈 */
#define STATE_HISTORY_CAPACITY 25u
#define STATE_HISTORY_MISSING ((UI_8)0xFFu) /* 历史中未找到 */

typedef struct StateHistory_t
{
    const StateType_t *states[STATE_HISTORY_CAPACITY]; /* 历史状态，自动迁移状态不入栈 */
    UI_8 size;                                        /* 有效条目数；栈顶下标为 size - 1 */
} StateHistory_t;

/*=========================================================
 * 2. 全局实例
 *=========================================================*/
extern StateInfo stateContext;
extern StateHistory_t stateHistory;

/*=========================================================
 * 3. 状态描述宏
 *=========================================================*/
#define STATE_SET_TRIGGER(t) (stateContext.trigger = (UI_16)(t))
#define STATE_TRIGGER() ((UI_16)stateContext.trigger)
#define STATE_CURRENT() (stateContext.state)
#define STATE_RULES(tbl) (UI_8)(sizeof(tbl) / sizeof((tbl)[0])), (tbl)

/*=========================================================
 * 4. 引擎接口
 *=========================================================*/
void StateEngineInit(const StateType_t *initialState);
void StateEngine(UI_16 keyCode);
void StateEngineRecovery(void);
UI_8 StateGetHistoryTop(void);

BOOL StateFindTable(const StateTable_t **pTable, UI_8 *pRuleCount, UI_16 *pTrigger);

#endif /* MAIN_STAT_H */
