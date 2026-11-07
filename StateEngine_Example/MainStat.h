#ifndef MAIN_STAT_H
#define MAIN_STAT_H

#include "TypeDefine_Sample.h" /* 工程基础类型与通用标志（实机替换此文件） */

/*=========================================================
 * 1. 数据结构
 *=========================================================*/

/* 迁移表中范围条件对应的触发值区间，包含两个端点 */
typedef struct TriggerRange_t
{
    UI_16 min;
    UI_16 max;
} TriggerRange_t;

/* 迁移表项的后处理返回值传给目标状态表的前处理；自动迁移时，前处理返回下一轮触发值 */
typedef UI_16 FUNC(UI_16);

/* 状态表类型前置声明 */
typedef struct StateType_t StateType_t;

/* 迁移表结构：每个表项保存迁移条件、后处理函数和目标状态表指针，多个表项组成数组 */
typedef struct StateTable_t
{
    UI_16 trigger;                /* 迁移条件：触发值、范围标识或 STATE_ANY */
    FUNC *onExit;                 /* 命中本表项后、切换状态表指针前执行的后处理函数 */
    const StateType_t *nextState; /* 迁移条件匹配后的目标状态表指针 */
} StateTable_t;

/* 状态表：保存前处理函数、自动迁移标志、迁移表条目数与数组指针、画面 ID */
struct StateType_t
{
    FUNC *onEnter;               /* 进入本状态表对应状态时执行的前处理函数 */
    UI_8 autoFlg : 2;            /* bit0=1：前处理返回值驱动查询本表的迁移表，且本状态表不入栈 */
    UI_8 transNum : 6;           /* 关联迁移表数组的条目数，最大 63 */
    const StateTable_t *transTbl; /* 本状态表关联的迁移表数组指针 */
    UI_16 screenId;              /* 画面 ID */
};

/* 引擎上下文 */
typedef struct StateInfo
{
    UI_16 trigger;            /* 当前触发值 */
    const StateType_t *state; /* 当前状态表指针 */
} StateInfo;

/* 状态迁移历史栈：保存状态表指针 */
#define STATE_HISTORY_CAPACITY 25u
#define STATE_HISTORY_MISSING ((UI_8)0xFFu) /* 历史中未找到 */

typedef struct StateHistory_t
{
    const StateType_t *states[STATE_HISTORY_CAPACITY]; /* 历史状态表指针；自动迁移状态表不入栈 */
    UI_8 top;                                        /* 有效条目数；栈顶下标为 top - 1 */
} StateHistory_t;

/*=========================================================
 * 2. 全局实例
 *=========================================================*/
extern StateInfo stateInfo;
extern StateHistory_t stateHistory;

/*=========================================================
 * 3. 状态表访问与迁移表初始化宏
 *=========================================================*/
#define SetStateTrig(triggerValue) (stateInfo.trigger = (UI_16)(triggerValue))
#define GetStateTrig() ((UI_16)stateInfo.trigger)
#define GetStatePtr() (stateInfo.state)
#define ListOf(ruleArray) (UI_8)(sizeof(ruleArray) / sizeof((ruleArray)[0])), (ruleArray)

/*=========================================================
 * 4. 引擎接口
 *=========================================================*/
void StateEngineInit(const StateType_t *initialState);
void StateEngine(UI_16 keyCode);
void StateEngineRecovery(void);
UI_8 StateGetHistoryTop(void);

BOOL StateFindTable(const StateTable_t **pTable, UI_8 *pTransCnt, UI_16 *pTrigger);

#endif /* MAIN_STAT_H */
