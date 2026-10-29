#include "MainStat.h"
#include "TriggerDefine_Sample.h" /* 工程侧触发器配置（实机替换此文件） */

/* 平台临界区保护（实机由显示或系统平台实现） */
void PlatformEnterCritical(void);
void PlatformExitCritical(void);

/*=========================================================
 * 全局状态
 *=========================================================*/
StateInfo stateInfo = {0u, 0};
StateHistory_t stateStack = {{0}, 0u};

/*=========================================================
 * 内部函数前置声明
 *=========================================================*/
/* 迁移规则访问 */
static const StateTable_t *GetTable(const StateType_t *state);
static UI_8 GetTableCount(const StateType_t *state);
static FUNC *GetAfFunc(const StateTable_t **pRule);
static const StateType_t *GetNextState(const StateTable_t **pRule);
static FUNC *GetBFfunc(const StateType_t *state);
static BOOL GetAutolFlg(const StateType_t *state);

/* 状态历史 */
static UI_8 FindInHistory(const StateType_t *state);
static void PushHistory(const StateType_t *state);
static const StateType_t *GetCurrentState(void);
static void PopHistory(void);
static const StateType_t *GetPreviousState(void);
static void SaveHistory(void);

/*=========================================================
 * 特殊迁移目标状态
 *=========================================================*/
static UI_16 StateNoop(UI_16 trigger)
{
    return trigger;
}

const StateType_t g_previousState = {
    StateNoop, NO, 0u, 0, SCREEN_ID_NONE};

const StateType_t g_reenterState = {
    StateNoop, NO, 0u, 0, SCREEN_ID_NONE};

/*=========================================================
 * 对外接口（按调用时序：初始化 → 运行 → 恢复 → 查询）
 *=========================================================*/

/* 初始化：设置开机起始状态并执行一次进入回调 */
void StateEngineInit(const StateType_t *initialState)
{
    stateInfo.trigger = 0u;
    stateInfo.state = initialState;

    stateStack.stackTop = 0u;

    if ((initialState != 0) && (initialState->bfFuncPtr != 0))
    {
        stateInfo.trigger = initialState->bfFuncPtr(stateInfo.trigger);
    }
}

/*
 * 主状态引擎。
 * keyCode 为本次按键，由迁移规则决定下一状态；
 * 迁移目标是 g_previousState 时，StateEngineRecovery 把 stateInfo.state 换成
 * 上一状态，再执行该状态的进入回调完成迁移。
 * 自动迁移状态（autoFlg != NO）继续 while 循环。
 */
void StateEngine(UI_16 keyCode)
{
    const StateTable_t *ruleCursor;
    FUNC *action;
    UI_8 ruleCount;
    BOOL found;
    UI_16 nextTrigger;

    STATESETTRIGGER(keyCode);

    while (1)
    {
        /* 准备 */
        ruleCursor = GetTable(stateInfo.state);
        ruleCount = GetTableCount(stateInfo.state);

        /* 分支：ruleCount 是数值，因此传入其地址 */
        found = StateFindTable(&ruleCursor, &ruleCount, &stateInfo.trigger);

        if (found == NG)
        {
            return;
        }

        /* 执行迁移规则的回调 */
        action = GetAfFunc(&ruleCursor);
        PlatformEnterCritical();
        nextTrigger = (*action)(STATE_GET_TRIGGER());
        PlatformExitCritical();

        stateInfo.state = GetNextState(&ruleCursor);
        StateEngineRecovery();

        SaveHistory();
        /* 执行目标状态的进入回调，完成状态迁移 */
        action = GetBFfunc(stateInfo.state);
        PlatformEnterCritical();
        nextTrigger = (*action)(nextTrigger);
        PlatformExitCritical();

        /* 非自动迁移状态到此返回 */
        if (GetAutolFlg(stateInfo.state) == NO)
        {
            return;
        }

        /* 自动迁移：进入回调的返回值成为下一轮触发值 */
        STATESETTRIGGER(nextTrigger);
    }
}

/* 状态恢复：返回类目标直接出栈，随后由引擎执行目标状态的进入回调 */
void StateEngineRecovery(void)
{
    if (stateInfo.state == &g_previousState)
    {
        stateInfo.state = GetPreviousState(); /* 从历史中取上一个状态 */
    }
    else if (stateInfo.state == &g_reenterState)
    {
        stateInfo.state = GetCurrentState(); /* 取当前状态 */
    }
}

/*
 * 触发器检索：
 * 1. 普通触发器按值匹配；
 * 2. STATE_RANGE_MIN～STATE_RANGE_MAX 转换成 g_triggerRanges 的范围；
 * 3. 按表内顺序返回第一个命中的规则。
 */
BOOL StateFindTable(const StateTable_t **pRule, UI_8 *pRuleCount, UI_16 *pTrigger)
{
    const StateTable_t *rule;
    UI_8 i;
    UI_16 rangeMin;
    UI_16 rangeMax;
    UI_16 rangeIndex;
    UI_16 trigger;

    /* 准备 */
    rule = *pRule;
    trigger = *pTrigger;

    /* 规则表检索 */
    for (i = 0u; i < *pRuleCount; i++)
    {
        /* 迁移条件范围 */
        if ((rule->trigger >= STATE_RANGE_MIN) && (rule->trigger <= STATE_RANGE_MAX))
        {
            rangeIndex = (UI_16)(rule->trigger - STATE_RANGE_MIN);
            rangeMin = g_triggerRanges[rangeIndex].min;
            rangeMax = g_triggerRanges[rangeIndex].max;
        }
        else
        {
            rangeMin = rule->trigger;
            rangeMax = rule->trigger;
        }

        /* 迁移条件比较 */
        if ((trigger >= rangeMin) && (trigger <= rangeMax))
        {
            *pRule = rule;
            return OK;
        }
        rule++;
    }

    return NG;
}

/* 状态历史深度（调试用） */
UI_8 StateGetHistoryTop(void)
{
    return stateStack.stackTop;
}

/*=========================================================
 * 内部实现
 *=========================================================*/

/*---------------- 迁移规则访问 ----------------*/

/* 获取迁移规则表 */
static const StateTable_t *GetTable(const StateType_t *state)
{
    return state->atTable;
}

/* 获取迁移规则数量 */
static UI_8 GetTableCount(const StateType_t *state)
{
    return state->transitionCount;
}

/* 获取迁移处理回调 */
static FUNC *GetAfFunc(const StateTable_t **pRule)
{
    return (*pRule)->afFuncPtr;
}

/* 获取下一状态 */
static const StateType_t *GetNextState(const StateTable_t **pRule)
{
    return (*pRule)->nextState;
}

/* 获取进入状态回调 */
static FUNC *GetBFfunc(const StateType_t *state)
{
    return state->bfFuncPtr;
}

/* 获取自动迁移标志 */
static BOOL GetAutolFlg(const StateType_t *state)
{
    return (state->autoFlg & 0x01u);
}

/*---------------- 状态历史 ----------------*/

/* 从栈顶向前扫描：已存在返回其位置，未找到返回 0xFF */
static UI_8 FindInHistory(const StateType_t *state)
{
    UI_8 i;

    for (i = stateStack.stackTop; i > 0u; i--)
    {
        if (stateStack.states[i - 1u] == state)
        {
            return (UI_8)(i - 1u);
        }
    }

    return STATE_HISTORY_NOT_FOUND;
}

static void PushHistory(const StateType_t *state)
{
    UI_8 pos;

    /* 自动迁移状态是过渡态，不作为返回目标 */
    if ((state->autoFlg & 0x01u) != 0u)
    {
        return;
    }

    pos = FindInHistory(state);
    if (pos != STATE_HISTORY_NOT_FOUND)
    {
        /* 已存储：栈顶回退到该位置，丢弃其后的历史 */
        stateStack.stackTop = (UI_8)(pos + 1u);
        return;
    }

    stateStack.states[stateStack.stackTop++] = state;
}

/* 取当前状态：栈顶保存的就是当前状态 */
static const StateType_t *GetCurrentState(void)
{
    if (stateStack.stackTop > 0u)
    {
        return stateStack.states[stateStack.stackTop - 1u];
    }
    return 0;
}

/* 弹出当前状态：栈底保留，不把栈弹空 */
static void PopHistory(void)
{
    if (stateStack.stackTop > 1u)
    {
        stateStack.stackTop--;
    }
}

/* 取上一状态：先弹出当前状态，再从更新后的栈顶取上一状态 */
static const StateType_t *GetPreviousState(void)
{
    PopHistory();
    return GetCurrentState();
}

/* 记录状态迁移历史：已存在则回退栈顶，不存在则压栈 */
static void SaveHistory(void)
{
    PushHistory(stateInfo.state);
}

/*---------------- 平台占位 ----------------*/

/* 回调和迁移执行期间的临界区保护（实机替换为平台实现） */
void PlatformEnterCritical(void)
{
}

void PlatformExitCritical(void)
{
}
