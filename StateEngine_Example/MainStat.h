#ifndef MAIN_STAT_H
#define MAIN_STAT_H

#include "TypeDefine_Sample.h" /* 工程基础类型与通用标志（实机替换此文件） */

/*=========================================================
 * 1. 数据结构
 *=========================================================*/

/* 触发器范围：范围触发器在对照表中的最小/最大触发值 */
typedef struct TriggerRange_t
{
    UI_16 min;
    UI_16 max;
} TriggerRange_t;

/* 回调类型：入参为触发值，返回下一轮触发值 */
typedef UI_16 FUNC(UI_16);

/* 状态类型前置声明 */
typedef struct StateType_t StateType_t;

/* 单条迁移规则 */
typedef struct StateTable_t
{
    UI_16 trigger;                /* 触发值；范围触发器时为范围基值 */
    FUNC *afFuncPtr;              /* 退出状态时处理函数 */
    const StateType_t *nextState; /* 迁移目标状态 */
} StateTable_t;

/* 状态迁移表 */
struct StateType_t
{
    FUNC *bfFuncPtr;              /* 进入状态时执行 */
    UI_8 autoFlg : 2;             /* bit0=1 表示自动迁移 */
    UI_8 transitionCount : 6;     /* 迁移规则数量，最大 63 */
    const StateTable_t *atTable;  /* 迁移规则表 */
    UI_16 screenId;               /* 画面 ID */
};

/* 引擎上下文 */
typedef struct StateInfo
{
    UI_16 trigger;                /* 当前触发值 */
    const StateType_t *state;     /* 当前状态 */
} StateInfo;

/* 状态历史栈 */
#define STATE_HISTORY_DEPTH 25u
#define STATE_HISTORY_NOT_FOUND ((UI_8)0xFFu) /* 历史中未找到 */

typedef struct StateHistory_t
{
    const StateType_t *states[STATE_HISTORY_DEPTH]; /* 状态历史 */
    UI_8 stackTop;                                  /* 有效条目数；栈顶下标为 stackTop - 1 */
} StateHistory_t;

/*=========================================================
 * 2. 全局实例
 *=========================================================*/
extern StateInfo stateInfo;
extern StateHistory_t stateStack;

/*=========================================================
 * 3. 状态描述宏
 *=========================================================*/
#define STATESETTRIGGER(t) (stateInfo.trigger = (UI_16)(t))
#define STATE_GET_TRIGGER() ((UI_16)stateInfo.trigger)
#define STATE_CURRENT() (stateInfo.state)
#define STATE_RULES(tbl) (UI_8)(sizeof(tbl) / sizeof((tbl)[0])), (tbl)

/*=========================================================
 * 4. 引擎接口
 *=========================================================*/
void StateEngineInit(const StateType_t *initialState);
void StateEngine(UI_16 keyCode);
void StateEngineRecovery(void);

BOOL StateFindTable(const StateTable_t **pRule, UI_8 *pRuleCount, UI_16 *pTrigger);

#endif /* MAIN_STAT_H */
