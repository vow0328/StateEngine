#include "MainStat.h"
#include "TriggerDefine_Sample.h" /* 工程侧触发器配置（实机替换此文件） */

/* 平台临界区保护（实机由平台实现） */
void PlatformEnterCritical(void);
void PlatformExitCritical(void);

/*=========================================================
 * 状态引擎上下文与迁移历史
 *=========================================================*/
StateInfo stateInfo = {0u, 0};
StateHistory_t stateStack = {{0}, 0u};

/*=========================================================
 * 内部函数前置声明
 *=========================================================*/
/* 状态及迁移规则访问 */
static const StateTable_t *GetTable(const StateType_t *state);
static UI_8 GetTableNum(const StateType_t *state);
static FUNC *GetAfFunc(const StateTable_t **pTable);
static const StateType_t *GetNextState(const StateTable_t **pTable);
static FUNC *GetBFfunc(const StateType_t *state);
static BOOL GetAutolFlg(const StateType_t *state);

/* 状态迁移历史 */
static UI_8 FindInHistory(const StateType_t *state);
static void PushHistory(const StateType_t *state);
static const StateType_t *GetCurrentState(void);
static void PopHistory(void);
static const StateType_t *GetPreviousState(void);
static void SaveHistory(void);

/*=========================================================
 * 特殊迁移目标：返回上一状态或重新进入当前历史状态
 *=========================================================*/
static UI_16 StateNoop(UI_16 trigger)
{
    return trigger;
}

const StateType_t PreviousState = {
    StateNoop,
    NO,
    0u,
    0,
    SCREEN_ID_NONE
};

const StateType_t g_reenterState = {
    StateNoop,
    NO,
    0u,
    0,
    SCREEN_ID_NONE
};

/*=========================================================
 * 状态引擎接口
 *=========================================================*/

/* 初始化当前状态并清空历史；起始状态不会入栈，也不会触发自动迁移 */
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
 * 使用 keyCode 作为触发值，在当前状态的迁移表中查找匹配规则。
 * 匹配后依次执行规则回调、确定目标状态、记录历史、执行目标状态进入回调。
 * 特殊目标由 StateEngineRecovery 转换为历史状态。
 * 目标状态启用自动迁移时，以进入回调的返回值作为下一轮触发值；
 * 未找到匹配规则或目标状态未启用自动迁移时返回。
 */
void StateEngine(UI_16 keyCode)
{
    const StateTable_t *TblPtr;
    FUNC *FuncPtr;
    UI_8 transitionNum;
    BOOL result;
    UI_16 nextTrigger;

    SetStateTrigger(keyCode);

    while (1)
    {
        /* 获取当前状态的迁移规则表及规则数量 */
        TblPtr = GetTable(stateInfo.state);
        transitionNum = GetTableNum(stateInfo.state);

        /* 按触发值查找第一条匹配的迁移规则 */
        result = StateFindTable(&TblPtr, &transitionNum, &stateInfo.trigger);

        if (result == NG)
        {
            return;
        }

        /* 在切换状态前执行匹配规则的回调 */
        FuncPtr = GetAfFunc(&TblPtr);
        PlatformEnterCritical();
        nextTrigger = (*FuncPtr)(STATE_GET_TRIGGER());
        PlatformExitCritical();

        stateInfo.state = GetNextState(&TblPtr);
        StateEngineRecovery();

        SaveHistory();
        /* 将规则回调的返回值传给目标状态的进入回调 */
        FuncPtr = GetBFfunc(stateInfo.state);
        PlatformEnterCritical();
        nextTrigger = (*FuncPtr)(nextTrigger);
        PlatformExitCritical();

        /* 检查目标状态：NO 返回等待外部事件，YES 继续查目标状态的规则 */
        if (GetAutolFlg(stateInfo.state) == NO)
        {
            return;
        }

        /* 自动迁移：进入回调的返回值成为下一轮触发值 */
        SetStateTrigger(nextTrigger);
    }
}

/* 将特殊迁移目标替换为历史中的上一状态或当前状态 */
void StateEngineRecovery(void)
{
    if (stateInfo.state == &PreviousState)
    {
        stateInfo.state = GetPreviousState(); /* 回退历史栈，栈底保留 */
    }
    else if (stateInfo.state == &g_reenterState)
    {
        stateInfo.state = GetCurrentState(); /* 取历史栈顶，不出栈 */
    }
}

/*
 * 在迁移规则表中按顺序查找触发值，返回第一条匹配的规则。
 * 普通规则按触发值精确匹配；范围规则通过 g_triggerRanges 查找包含区间。
 * 匹配时更新 *pTable 并返回 OK；未匹配时返回 NG。
 */
BOOL StateFindTable(const StateTable_t **pTable, UI_8 *pRuleCount, UI_16 *pTrigger)
{
    const StateTable_t *ptr;
    UI_8 i;
    UI_16 rangeMin;
    UI_16 rangeMax;
    UI_16 rangeIndex;
    UI_16 trigger;

    /* 读取规则表和本次触发值 */
    ptr = *pTable;
    trigger = *pTrigger;

    /* 按规则表顺序查找 */
    for (i = 0u; i < *pRuleCount; i++)
    {
        /* 将范围标识映射为区间；普通触发值对应单值区间 */
        if ((ptr->trigger >= STATE_RANGE_MIN) && (ptr->trigger <= STATE_RANGE_MAX))
        {
            rangeIndex = (UI_16)(ptr->trigger - STATE_RANGE_MIN);
            rangeMin = g_triggerRanges[rangeIndex].min;
            rangeMax = g_triggerRanges[rangeIndex].max;
        }
        else
        {
            rangeMin = ptr->trigger;
            rangeMax = ptr->trigger;
        }

        /* 判断触发值是否落在规则区间内 */
        if ((trigger >= rangeMin) && (trigger <= rangeMax))
        {
            *pTable = ptr;
            return OK;
        }

        ptr++;
    }

    return NG;
}

/* 获取当前历史栈的有效条目数（调试用） */
UI_8 StateGetHistoryTop(void)
{
    return stateStack.stackTop;
}

/*=========================================================
 * 内部实现
 *=========================================================*/

/*---------------- 状态及迁移规则访问 ----------------*/

/* 获取迁移规则表 */
static const StateTable_t *GetTable(const StateType_t *state)
{
    return state->atTable;
}

/* 获取迁移规则数量 */
static UI_8 GetTableNum(const StateType_t *state)
{
    return state->transitionNum;
}

/* 获取匹配规则的迁移回调 */
static FUNC *GetAfFunc(const StateTable_t **pTable)
{
    return (*pTable)->afFuncPtr;
}

/* 获取规则指定的目标状态 */
static const StateType_t *GetNextState(const StateTable_t **pTable)
{
    return (*pTable)->nextState;
}

/* 获取目标状态的进入回调 */
static FUNC *GetBFfunc(const StateType_t *state)
{
    return state->bfFuncPtr;
}

/* 获取自动迁移标志 */
static BOOL GetAutolFlg(const StateType_t *state)
{
    return (state->autoFlg & 0x01u);
}

/*---------------- 状态迁移历史 ----------------*/

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

    /* 当前实现不检查栈满，调用方必须保证历史不超过 STATE_HISTORY_DEPTH */
    stateStack.states[stateStack.stackTop++] = state;
}

/* 获取历史栈顶状态；栈为空时返回 0 */
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

/* 弹出历史栈顶后获取新的栈顶；栈底保留 */
static const StateType_t *GetPreviousState(void)
{
    PopHistory();
    return GetCurrentState();
}

/* 记录目标状态：自动迁移状态不入栈；已存在则回退，否则压栈 */
static void SaveHistory(void)
{
    PushHistory(stateInfo.state);
}

/*---------------- 平台占位 ----------------*/

/* 规则回调和状态进入回调的临界区保护（实机替换为平台实现） */
void PlatformEnterCritical(void)
{
}

void PlatformExitCritical(void)
{
}
