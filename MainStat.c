#include "MainStat.h"
#include <stddef.h>

/* 平台临界区保护（实机由平台实现） */
void PlatformEnterCritical(void);
void PlatformExitCritical(void);

/*=========================================================
 * 状态引擎上下文与迁移历史
 *=========================================================*/
StateInfo stateInfo = {0u, 0};
StateHistory_t stateHistory = {{0}, 0u};

/*=========================================================
 * 内部函数前置声明
 *=========================================================*/
/* 状态表与迁移表访问 */
static const StateTable_t *GetTable(const StateType_t *state);
static UI_8 GetTransCnt(const StateType_t *state);
static FUNC *GetOnExit(const StateTable_t **pTable);
static const StateType_t *GetNextState(const StateTable_t **pTable);
static FUNC *GetOnEnter(const StateType_t *state);
static BOOL IsAutoState(const StateType_t *state);

/* 状态迁移历史 */
static UI_8 FindInHistory(const StateType_t *state);
static void PushHistory(const StateType_t *state);
static const StateType_t *GetCurrentState(void);
static void PopHistory(void);
static const StateType_t *GetPreviousState(void);
static void SaveHistory(void);

/*=========================================================
 * 特殊状态表占位：返回上一历史状态表，或重新进入历史栈顶状态表
 *=========================================================*/

const StateType_t PreviousState = {
    NULL,
    NO,
    0u,
    0,
    SCREEN_ID_NONE};

const StateType_t FormEntryState = {
    NULL,
    NO,
    0u,
    0,
    SCREEN_ID_NONE};

/*=========================================================
 * 状态引擎接口
 *=========================================================*/

/* 初始化当前状态表指针并清空历史；不执行前处理、不自动迁移，也不将初始状态表入栈 */
void StateEngineInit(const StateType_t *initialState)
{
    stateInfo.trigger = 0u;
    stateInfo.state = initialState;

    stateHistory.top = 0u;
}

/*
 * 使用 keyCode 作为触发值，查询当前状态表的 transTbl 指向的迁移表数组。
 * 命中表项后，依次执行其后处理、切换状态表指针、记录历史、执行目标状态表的前处理。
 * 特殊状态表占位由 StateEngineRecovery 解析为历史中的状态表指针。
 * 目标状态表启用自动迁移时，以其前处理返回值作为下一轮迁移表查询的触发值；
 * 未命中迁移表项，或目标状态表未启用自动迁移时返回。
 */
void StateEngine(UI_16 keyCode)
{
    const StateTable_t *TblPtr;
    FUNC *FuncPtr;
    UI_8 transNum;
    BOOL result;
    UI_16 nextTrigger;

    SetStateTrig(keyCode);

    while (1)
    {
        /* 从当前状态表读取迁移表数组首地址与条目数 */
        TblPtr = GetTable(stateInfo.state);
        transNum = GetTransCnt(stateInfo.state);

        /* 按触发值查找第一个满足迁移条件的表项 */
        result = StateFindTable(&TblPtr, &transNum, &stateInfo.trigger);

        if (result == NG)
        {
            return;
        }

        /* 切换状态表指针前，执行命中迁移表项的后处理函数 */
        FuncPtr = GetOnExit(&TblPtr);
        PlatformEnterCritical();
        nextTrigger = (*FuncPtr)(GetStateTrig());
        PlatformExitCritical();

        /* 切换到迁移表项指定的目标状态表，再解析特殊状态表占位。 */
        stateInfo.state = GetNextState(&TblPtr);
        StateEngineRecovery();

        SaveHistory();
        /* 将迁移表项后处理的返回值传给目标状态表的前处理函数 */
        FuncPtr = GetOnEnter(stateInfo.state);
        PlatformEnterCritical();
        nextTrigger = (*FuncPtr)(nextTrigger);
        PlatformExitCritical();

        /* 检查目标状态表：NO 返回等待外部事件，YES 继续查询其迁移表 */
        if (IsAutoState(stateInfo.state) == NO)
        {
            return;
        }

        /* 自动迁移：前处理函数的返回值成为下一轮触发值 */
        SetStateTrig(nextTrigger);
    }
}

/* 将特殊状态表占位替换为上一历史状态表或当前历史栈顶状态表的指针 */
void StateEngineRecovery(void)
{
    if (stateInfo.state == &PreviousState)
    {
        stateInfo.state = GetPreviousState(); /* 回退历史栈，栈底保留 */
    }
    else if (stateInfo.state == &FormEntryState)
    {
        stateInfo.state = GetCurrentState(); /* 取历史栈顶，不出栈 */
    }
}

/*
 * 按迁移表数组的顺序检查条件，选取第一个匹配触发值的表项。
 * 普通迁移条件精确匹配；范围条件通过 TriggerRanges 查询区间；STATE_ANY 无条件匹配。
 * 命中时将 *pTable 指向该迁移表项并返回 OK；未命中时返回 NG，不切换状态表指针。
 */
BOOL StateFindTable(const StateTable_t **pTable, UI_8 *pTransCnt, UI_16 *pTrigger)
{
    const StateTable_t *ptr;
    UI_16 rangeMin;
    UI_16 rangeMax;
    UI_16 rangeIndex;
    UI_16 trigger;

    /* 读取迁移表数组首地址和本次触发值 */
    ptr = *pTable;
    trigger = *pTrigger;

    /* 按迁移表数组顺序查找 */
    for (UI_8 i = 0u; i < *pTransCnt; i++)
    {
        if (ptr->trigger == STATE_ANY)
        {
            *pTable = ptr;
            return OK;
        }
        /* 将范围标识映射为区间；普通触发值对应单值区间 */
        if ((ptr->trigger >= STATE_RANGE_MIN) && (ptr->trigger <= STATE_RANGE_MAX))
        {
            rangeIndex = (UI_16)(ptr->trigger - STATE_RANGE_MIN);
            rangeMin = TriggerRanges[rangeIndex].min;
            rangeMax = TriggerRanges[rangeIndex].max;
        }
        else
        {
            rangeMin = ptr->trigger;
            rangeMax = ptr->trigger;
        }

        /* 判断触发值是否满足本迁移表项的区间条件 */
        if ((trigger >= rangeMin) && (trigger <= rangeMax))
        {
            *pTable = ptr;
            return OK;
        }

        ptr++;
    }

    return NG;
}

/*=========================================================
 * 内部实现
 *=========================================================*/

/*---------------- 状态表与迁移表访问 ----------------*/

/* 读取状态表中的迁移表数组指针 */
static const StateTable_t *GetTable(const StateType_t *state)
{
    return state->transTbl;
}

/* 读取状态表中的迁移表条目数 */
static UI_8 GetTransCnt(const StateType_t *state)
{
    return state->transNum;
}

/* 读取命中迁移表项的后处理函数 */
static FUNC *GetOnExit(const StateTable_t **pTable)
{
    return (*pTable)->onExit;
}

/* 读取迁移表项指定的目标状态表指针 */
static const StateType_t *GetNextState(const StateTable_t **pTable)
{
    return (*pTable)->nextState;
}

/* 读取目标状态表的前处理函数 */
static FUNC *GetOnEnter(const StateType_t *state)
{
    return state->onEnter;
}

/* 读取状态表的自动迁移标志 */
static BOOL IsAutoState(const StateType_t *state)
{
    return (state->autoFlg & 0x01u);
}

/*---------------- 状态迁移历史 ----------------*/

UI_8 StateGetHistoryTop(void)
{
    return stateHistory.top;
}

/* 从栈顶向前查找状态表指针：找到返回下标，未找到返回 0xFF */
static UI_8 FindInHistory(const StateType_t *state)
{
    UI_8 i;

    for (i = stateHistory.top; i > 0u; i--)
    {
        if (stateHistory.states[i - 1u] == state)
        {
            return (UI_8)(i - 1u);
        }
    }

    return STATE_HISTORY_MISSING;
}

static void PushHistory(const StateType_t *state)
{
    UI_8 pos;

    /* 启用自动迁移的状态表不入历史栈，不作为历史返回目标 */
    if ((state->autoFlg & 0x01u) != 0u)
    {
        return;
    }

    pos = FindInHistory(state);
    if (pos != STATE_HISTORY_MISSING)
    {
        /* 已存储：栈顶回退到该位置，丢弃其后的历史 */
        stateHistory.top = (UI_8)(pos + 1u);
        return;
    }

    /* 当前实现不检查栈满，调用方必须保证历史不超过 STATE_HISTORY_CAPACITY */
    stateHistory.states[stateHistory.top++] = state;
}

/* 获取历史栈顶保存的状态表指针；栈为空时返回 0 */
static const StateType_t *GetCurrentState(void)
{
    if (stateHistory.top > 0u)
    {
        return stateHistory.states[stateHistory.top - 1u];
    }

    return 0;
}

/* 移除当前历史状态表指针；保留栈底，不把栈弹空 */
static void PopHistory(void)
{
    if (stateHistory.top > 1u)
    {
        stateHistory.top--;
    }
}

/* 弹出当前历史状态表指针后取新的栈顶指针；栈底保留 */
static const StateType_t *GetPreviousState(void)
{
    PopHistory();
    return GetCurrentState();
}

/* 记录目标状态表指针：自动迁移状态表不入栈；已存在则截断历史，否则压栈 */
static void SaveHistory(void)
{
    PushHistory(stateInfo.state);
}

/*---------------- 平台占位 ----------------*/

/* 后处理函数和前处理函数的临界区保护（实机替换为平台实现） */
void PlatformEnterCritical(void)
{
}

void PlatformExitCritical(void)
{
}
