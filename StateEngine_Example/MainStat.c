#include "MainStat.h"
#include "TriggerDefine_Sample.h" /* 工程侧触发器配置（实机替换此文件） */

/* 平台临界区保护（实机由显示平台实现） */
void API_DISPLAY_GetChkSumSum(void);
void API_DISPLAY_RetChkSumSum(void);

/*=========================================================
 * 全局状态
 *=========================================================*/
StateInfoType_t stateInfo = {0u, 0};

#define MAIN_STATE_STACK_DEPTH 25u
#define MAIN_STATE_STACK_NOT_FOUND ((UI_8)0xFFu) /* 栈中未找到 */

/* 状态历史栈：数组与栈顶位置统一管理 */
typedef struct StateStackType_t
{
    const StateType_t *pState[MAIN_STATE_STACK_DEPTH]; /* 状态历史 */
    UI_8 stackTop;                                     /* 栈顶位置：有效条目数，栈顶下标为 stackTop - 1 */
} StateStackType_t;

static StateStackType_t stateStack = {{0}, 0u};

/*=========================================================
 * 内部函数前置声明
 *=========================================================*/
/* 迁移表访问 */
static const StateTableParts_t *GetTbl(const StateType_t *ptr);
static UI_8 GetTblTrigNum(const StateType_t *ptr);
static FUNC *GetAPFunc(const StateTableParts_t **ptr);
static const StateType_t *GetNextTbl(const StateTableParts_t **ptr);
static FUNC *GetBFfunc(const StateType_t *ptr);
static BOOL GetTblFlg(const StateType_t *ptr);

/* 状态历史栈 */
static UI_8 FindStateInStack(const StateType_t *state);
static void PushState(const StateType_t *state);
static const StateType_t *GetCurrentState(void);
static void PopState(void);
static const StateType_t *GetPreviousState(void);
static void SaveStateTbl(void);

/*=========================================================
 * 特殊迁移目标状态
 *=========================================================*/
static UI_16 DummyFunc(UI_16 trig)
{
    return trig;
}

const StateType_t TblPreviousState = {
    DummyFunc, NO, 0u, 0, PT_SCRN_DUMMY};

const StateType_t TblFormEntryState = {
    DummyFunc, NO, 0u, 0, PT_SCRN_DUMMY};

/*=========================================================
 * 对外接口（按调用时序：初始化 → 运行 → 恢复 → 查询）
 *=========================================================*/

/* 初始化：设置开机起始状态并执行一次入口处理 */
void MainStateEngineInit(const StateType_t *initialState)
{
    stateInfo.trig = 0u;
    stateInfo.tblPtr = initialState;

    stateStack.stackTop = 0u;

    if ((initialState != 0) && (initialState->bfFuncPtr != 0))
    {
        stateInfo.trig = initialState->bfFuncPtr(stateInfo.trig);
    }
}

/*
 * 主状态引擎。
 * keycode 为本次按键，由迁移表决定下一状态；
 * 迁移目标是 TblPreviousState 时，Recovery 把 stateInfo.tblPtr 换成上一状态，
 * 再执行该状态的前处理函数完成迁移。
 * 自动迁移状态（reqFlg != NO）继续 while 循环。
 */
void MainStateEngine(UI_16 keycode)
{
    const StateTableParts_t *tblPtr_R;
    FUNC *funcPtr;
    UI_8 pTrigNum;
    BOOL result;
    UI_16 retFuncPtr;

    SetStateTrig(keycode);

    while (1)
    {
        /* 準備 */
        tblPtr_R = GetTbl(stateInfo.tblPtr);
        pTrigNum = GetTblTrigNum(stateInfo.tblPtr);

        /* 分岐：pTrigNum 是数值，因此传入其地址 */
        result = GetTrigTbl(&tblPtr_R, &pTrigNum, &stateInfo.trig);

        if (result == NG)
        {
            return;
        }

        /* 本体処理：执行迁移表条目的 atFuncPtr */
        funcPtr = GetAPFunc(&tblPtr_R);
        API_DISPLAY_GetChkSumSum();
        retFuncPtr = (*funcPtr)(GetStateTrig());
        API_DISPLAY_RetChkSumSum();

        stateInfo.tblPtr = GetNextTbl(&tblPtr_R);
        MainStateEngineRecovery();

        SaveStateTbl();
        /* 执行目标状态的前处理函数，完成状态迁移 */
        funcPtr = GetBFfunc(stateInfo.tblPtr);
        API_DISPLAY_GetChkSumSum();
        retFuncPtr = (*funcPtr)(retFuncPtr);
        API_DISPLAY_RetChkSumSum();

        /* 非自动迁移状态到此返回 */
        if (GetTblFlg(stateInfo.tblPtr) == NO)
        {
            return;
        }

        /* 自动迁移：入口回调返回值成为下一轮触发器 */
        SetStateTrig(retFuncPtr);
    }
}

/* 状態恢复：返回类目标直接出栈，随后由引擎执行目标状态的前处理函数 */
void MainStateEngineRecovery(void)
{
    if (stateInfo.tblPtr == &TblPreviousState)
    {
        stateInfo.tblPtr = GetPreviousState(); // 从历史中取上一个状态
    }
    else if (stateInfo.tblPtr == &TblFormEntryState)
    {
        stateInfo.tblPtr = GetCurrentState(); // 取当前状态
    }
}

/*
 * 触发器检索：
 * 1. 普通触发器按值匹配；
 * 2. MC_TOP～MC_END 转换成 TblMcTrig 的范围；
 * 3. 按表内顺序返回第一个命中的条目。
 */
BOOL GetTrigTbl(const StateTableParts_t **pTbl, UI_8 *pTrigNum, UI_16 *pOrgTrig)
{
    const StateTableParts_t *ptr;
    UI_8 i;
    UI_16 trigMin;
    UI_16 trigMax;
    UI_16 trigTemp;
    UI_16 trig;
    /* 準備 */
    ptr = *pTbl;
    trig = *pOrgTrig;
    /* テーブル検索 */
    for (i = 0u; i < *pTrigNum; i++)
    {
        /* 遷移情報範囲 */
        if ((ptr->trig >= MC_TOP) && (ptr->trig <= MC_END))
        {
            trigTemp = (UI_16)(ptr->trig - MC_TOP);
            trigMin = TblMcTrig[trigTemp].min;
            trigMax = TblMcTrig[trigTemp].max;
        }
        else
        {
            trigMin = ptr->trig;
            trigMax = ptr->trig;
        }

        /* 遷移条件比較 */
        if ((trig >= trigMin) && (trig <= trigMax))
        {
            *pTbl = ptr;
            return OK;
        }
        ptr++;
    }

    return NG;
}

/* 状态栈深度（调试用） */
UI_8 GetStateStackDepth(void)
{
    return stateStack.stackTop;
}

/*=========================================================
 * 内部实现
 *=========================================================*/

/*---------------- 迁移表访问 ----------------*/

/* 获取后半表格：对应原工程辅助函数 GetTbl() */
static const StateTableParts_t *GetTbl(const StateType_t *ptr)
{
    return ptr->pR_Tbl;
}

/* 获取触发数量 */
static UI_8 GetTblTrigNum(const StateType_t *ptr)
{
    return ptr->trTrigNum;
}

/* 获取迁移/退状态处理函数：保留原字段 atFuncPtr */
static FUNC *GetAPFunc(const StateTableParts_t **ptr)
{
    return (*ptr)->atFuncPtr;
}

/* 获取下一状态 */
static const StateType_t *GetNextTbl(const StateTableParts_t **ptr)
{
    return (*ptr)->nextTblPtr;
}

/* 获取进入状态处理函数：保留原字段 bfFuncPtr */
static FUNC *GetBFfunc(const StateType_t *ptr)
{
    return ptr->bfFuncPtr;
}

/* 获取状态的自动迁移标志 */
static BOOL GetTblFlg(const StateType_t *ptr)
{
    return (ptr != 0) ? ((ptr->reqFlg & 0x01u) != 0u) : FALSE;
}

/*---------------- 状态历史栈 ----------------*/

/* 从栈顶向前扫描：已存在返回其位置，未找到返回 0xFF */
static UI_8 FindStateInStack(const StateType_t *state)
{
    UI_8 i;

    for (i = stateStack.stackTop; i > 0u; i--)
    {
        if (stateStack.pState[i - 1u] == state)
        {
            return (UI_8)(i - 1u);
        }
    }

    return MAIN_STATE_STACK_NOT_FOUND;
}

static void PushState(const StateType_t *state)
{
    UI_8 pos;

    /* 自动迁移状态是过渡态，不作为返回目标 */
    if ((state->reqFlg & 0x01u) != 0u)
    {
        return;
    }

    pos = FindStateInStack(state);
    if (pos != MAIN_STATE_STACK_NOT_FOUND)
    {
        /* 已存储：栈顶回退到该位置，丢弃其后的历史 */
        stateStack.stackTop = (UI_8)(pos + 1u);
        return;
    }

    stateStack.pState[stateStack.stackTop++] = state;
}

/* 取当前状态：栈顶保存的就是当前状态 */
static const StateType_t *GetCurrentState(void)
{
    if (stateStack.stackTop > 0u)
    {
        return stateStack.pState[stateStack.stackTop - 1u];
    }
    return 0;
}

/* 弹出当前状态：栈底保留，不把栈弹空 */
static void PopState(void)
{
    if (stateStack.stackTop > 1u)
    {
        stateStack.stackTop--;
    }
}

/* 取上一状态：先弹出当前状态，再从更新后的栈顶取上一状态 */
static const StateType_t *GetPreviousState(void)
{
    PopState();
    return GetCurrentState();
}

/* 记录状态迁移历史：已存在则回退栈顶，不存在则压栈（见 PushState） */
static void SaveStateTbl(void)
{
    PushState(stateInfo.tblPtr);
}

/*---------------- 平台占位 ----------------*/

/* 原工程在回调执行期间做临界区保护 */
void API_DISPLAY_GetChkSumSum(void)
{
}

void API_DISPLAY_RetChkSumSum(void)
{
}
