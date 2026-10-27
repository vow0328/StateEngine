#ifndef MAIN_STAT_H
#define MAIN_STAT_H

#include "TypeDefine_Sample.h" /* 工程基础类型与通用标志（实机替换此文件） */

/*=========================================================
 * 1. 数据结构
 *=========================================================*/

/* 最大最小值触发器范围（范围触发器的查找表条目） */
typedef struct MinToMaxTrig_T
{
    UI_16 min;
    UI_16 max;
} MinToMaxTrig_T;

typedef UI_16 FUNC(UI_16); /* 関数指針型 */

typedef struct StateType_t StateType_t;

/* 状態遷移表末端部分（遷移条目） */
typedef struct StateTableParts_t
{
    UI_16 trig;                    /* 遷移条件（触発値或範囲） */
    FUNC /*const*/ *atFuncPtr;     /* 退状態処理関数指針 */
    const StateType_t *nextTblPtr; /* 次の状態指針 */
} StateTableParts_t;

/* 状態遷移表（一个状态） */
struct StateType_t
{
    FUNC /*const*/ *bfFuncPtr;       /* 入状態処理関数指針 */
    UI_8 reqFlg : 2;                 /* 自動遷移号（1=継続循環、0=停止） */
    UI_8 trTrigNum : 6;              /* 遷移テーブル条目数（最大63） */
    const StateTableParts_t *pR_Tbl; /* 後半テーブル指針 */
    UI_16 scrnId;                    /* スクリーンID */
};

/* 引擎全局状态 */
struct StateInfoType
{
    UI_16 trig;                /* 当前触发器 */
    const StateType_t *tblPtr; /* 当前状态指针 */
};
typedef struct StateInfoType StateInfoType_t;

/*=========================================================
 * 2. 全局实例
 *=========================================================*/
extern StateInfoType_t stateInfo;

/*=========================================================
 * 3. 状态描述宏
 *=========================================================*/
#define SetStateTrig(t) (stateInfo.trig = (UI_16)(t))
#define GetStateTrig() ((UI_16)stateInfo.trig)
#define GetStateTbl() (stateInfo.tblPtr)
#define ListOf(tbl) (UI_8)(sizeof(tbl) / sizeof((tbl)[0])), (tbl)

/*=========================================================
 * 4. 状态迁移引擎接口
 *=========================================================*/
void MainStateEngineInit(const StateType_t *initialState);
void MainStateEngine(UI_16 keycode);
void MainStateEngineRecovery(void);

BOOL GetTrigTbl(const StateTableParts_t **pTbl, UI_8 *pTrigNum, UI_16 *pOrgTrig);

#endif /* MAIN_STAT_H */
