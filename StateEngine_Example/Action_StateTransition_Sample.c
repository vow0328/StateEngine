#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

/*=========================================================
 * 井字棋：带画面跳转和人机对战的状态机示例
 *
 * MENU --Enter--> MENU_DECIDE --自动--> X_TURN / QUIT
 * X_TURN / O_TURN --方向键--> 自身（重绘）
 * 双人：X_TURN / O_TURN --Enter--> CHECK --自动--> 下一回合 / 胜负 / 平局
 * 人机：X_TURN --Enter--> CHECK --自动--> COMPUTER_TURN --自动--> CHECK ...
 * X_WIN / O_WIN / DRAW --Enter--> RESULT_DECIDE --自动--> X_TURN / MENU
 * 任意游戏画面 --Esc--> MENU
 *
 * 入口回调只更新数据，主循环在引擎返回后按 screenId 渲染。
 *=========================================================*/

#define TTT_EMPTY  ((char)0)
#define TTT_MARK_X ((char)'X')
#define TTT_MARK_O ((char)'O')

static const UI_8 g_winLines[8][3] =
{
    {0u, 1u, 2u}, {3u, 4u, 5u}, {6u, 7u, 8u},
    {0u, 3u, 6u}, {1u, 4u, 7u}, {2u, 5u, 8u},
    {0u, 4u, 8u}, {2u, 4u, 6u},
};

static char g_board[9];
static UI_8 g_moveCount;        /* 已落子数量 */
static char g_currentMover;     /* 当前回合该谁落子 */
static char g_lastMover;        /* 最近一次成功落子的人 */
static UI_8 g_cursor = 4u;      /* 棋盘光标，默认中间格 */
static UI_8 g_menuSel = TTT_MENU_2P;
static UI_8 g_resultSel = TTT_RESULT_REPLAY;
static UI_8 g_gameMode = TTT_MODE_2P;
static const char *g_notice;    /* 画面上的提示信息 */

/*=========================================================
 * 棋盘辅助
 *=========================================================*/

static void TttResetBoard(void)
{
    UI_8 i;

    for (i = 0u; i < 9u; i++)
    {
        g_board[i] = TTT_EMPTY;
    }

    g_moveCount = 0u;
    g_currentMover = TTT_EMPTY;
    g_lastMover = TTT_EMPTY;
    g_cursor = 4u;
    g_resultSel = TTT_RESULT_REPLAY;
    g_notice = 0;
}

static BOOL TttHasLine(char mark)
{
    UI_8 i;

    for (i = 0u; i < 8u; i++)
    {
        if ((g_board[g_winLines[i][0]] == mark) &&
            (g_board[g_winLines[i][1]] == mark) &&
            (g_board[g_winLines[i][2]] == mark))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/* 落子：位置已被占用时返回 FALSE，棋盘不变 */
static BOOL TttPlaceMark(char mark, UI_8 cell)
{
    if (g_board[cell] != TTT_EMPTY)
    {
        g_notice = "该位置已有棋子，请重新选择。";
        return FALSE;
    }

    g_board[cell] = mark;
    g_moveCount++;
    g_lastMover = mark;
    g_notice = 0;

    return TRUE;
}

/* 找到某方已经有两子、第三格为空的位置 */
static BOOL TttFindWinningCell(char mark, UI_8 *pCell)
{
    UI_8 i;
    UI_8 j;
    UI_8 empty;
    UI_8 count;

    for (i = 0u; i < 8u; i++)
    {
        empty = 9u;
        count = 0u;

        for (j = 0u; j < 3u; j++)
        {
            if (g_board[g_winLines[i][j]] == mark)
            {
                count++;
            }
            else if (g_board[g_winLines[i][j]] == TTT_EMPTY)
            {
                empty = g_winLines[i][j];
            }
        }

        if ((count == 2u) && (empty < 9u))
        {
            *pCell = empty;
            return TRUE;
        }
    }

    return FALSE;
}

/* 电脑简单策略：能赢则赢、能堵则堵，其次中心、角、边 */
static UI_8 TttChooseComputerCell(void)
{
    static const UI_8 corners[4] = {0u, 2u, 6u, 8u};
    static const UI_8 sides[4] = {1u, 3u, 5u, 7u};
    UI_8 cell;
    UI_8 i;

    if (TttFindWinningCell(TTT_MARK_O, &cell) != FALSE)
    {
        return cell;
    }

    if (TttFindWinningCell(TTT_MARK_X, &cell) != FALSE)
    {
        return cell;
    }

    if (g_board[4] == TTT_EMPTY)
    {
        return 4u;
    }

    for (i = 0u; i < 4u; i++)
    {
        if (g_board[corners[i]] == TTT_EMPTY)
        {
            return corners[i];
        }
    }

    for (i = 0u; i < 4u; i++)
    {
        if (g_board[sides[i]] == TTT_EMPTY)
        {
            return sides[i];
        }
    }

    return 9u;
}

/*=========================================================
 * 状态入口函数（bfFuncPtr）
 *=========================================================*/

/* 空处理：入口和迁移动作共用 */
static UI_16 TttNoop(UI_16 trigger)
{
    return trigger;
}

static UI_16 TttXTurnEntry(UI_16 trigger)
{
    g_currentMover = TTT_MARK_X;
    return trigger;
}

static UI_16 TttOTurnEntry(UI_16 trigger)
{
    g_currentMover = TTT_MARK_O;
    return trigger;
}

/* 自动迁移状态：电脑执 O，落子后交给 CHECK 继续判定 */
static UI_16 TttComputerTurnEntry(UI_16 trigger)
{
    UI_8 cell;

    (void)trigger;
    g_currentMover = TTT_MARK_O;
    cell = TttChooseComputerCell();

    if (cell >= 9u)
    {
        g_resultSel = TTT_RESULT_REPLAY;
        return TRIG_TTT_DRAW;
    }

    (void)TttPlaceMark(TTT_MARK_O, cell);
    return TRIG_TTT_COMPUTER_DONE;
}

/* 自动迁移状态：检查胜负并生成下一轮触发器 */
static UI_16 TttCheckEntry(UI_16 trigger)
{
    (void)trigger;

    if (TttHasLine(TTT_MARK_X) != FALSE)
    {
        g_resultSel = TTT_RESULT_REPLAY;
        return TRIG_TTT_X_WIN;
    }

    if (TttHasLine(TTT_MARK_O) != FALSE)
    {
        g_resultSel = TTT_RESULT_REPLAY;
        return TRIG_TTT_O_WIN;
    }

    if (g_moveCount >= 9u)
    {
        g_resultSel = TTT_RESULT_REPLAY;
        return TRIG_TTT_DRAW;
    }

    /* 本回合没有成功落子（位置已被占用），留在原回合 */
    if (g_lastMover != g_currentMover)
    {
        if (g_currentMover == TTT_MARK_X)
        {
            return TRIG_TTT_TO_X;
        }

        if (g_gameMode == TTT_MODE_2P)
        {
            return TRIG_TTT_TO_O;
        }

        /* 电脑回合落子失败理论上不会发生；回到玩家回合，避免死循环 */
        return TRIG_TTT_TO_X;
    }

    if (g_currentMover == TTT_MARK_X)
    {
        return (g_gameMode == TTT_MODE_CPU) ? TRIG_TTT_TO_CPU : TRIG_TTT_TO_O;
    }

    return TRIG_TTT_TO_X;
}

static UI_16 TttMenuDecideEntry(UI_16 trigger)
{
    (void)trigger;

    if (g_menuSel == TTT_MENU_2P)
    {
        return TRIG_TTT_START_2P;
    }

    if (g_menuSel == TTT_MENU_CPU)
    {
        return TRIG_TTT_START_CPU;
    }

    return TRIG_TTT_QUIT;
}

static UI_16 TttResultDecideEntry(UI_16 trigger)
{
    (void)trigger;
    return (g_resultSel == TTT_RESULT_REPLAY) ? TRIG_TTT_REPLAY : TRIG_TTT_MENU;
}

/*=========================================================
 * 迁移/退状态处理函数（afFuncPtr）
 *=========================================================*/

static UI_16 TttMenuUpAction(UI_16 trigger)
{
    if (g_menuSel > TTT_MENU_2P)
    {
        g_menuSel--;
    }
    return trigger;
}

static UI_16 TttMenuDownAction(UI_16 trigger)
{
    if (g_menuSel < TTT_MENU_QUIT)
    {
        g_menuSel++;
    }
    return trigger;
}

static UI_16 TttResultUpAction(UI_16 trigger)
{
    if (g_resultSel > TTT_RESULT_REPLAY)
    {
        g_resultSel--;
    }
    return trigger;
}

static UI_16 TttResultDownAction(UI_16 trigger)
{
    if (g_resultSel < TTT_RESULT_MENU)
    {
        g_resultSel++;
    }
    return trigger;
}

static UI_16 TttMoveUpAction(UI_16 trigger)
{
    if (g_cursor >= 3u)
    {
        g_cursor = (UI_8)(g_cursor - 3u);
    }
    g_notice = 0;
    return trigger;
}

static UI_16 TttMoveDownAction(UI_16 trigger)
{
    if (g_cursor <= 5u)
    {
        g_cursor = (UI_8)(g_cursor + 3u);
    }
    g_notice = 0;
    return trigger;
}

static UI_16 TttMoveLeftAction(UI_16 trigger)
{
    if ((g_cursor % 3u) != 0u)
    {
        g_cursor--;
    }
    g_notice = 0;
    return trigger;
}

static UI_16 TttMoveRightAction(UI_16 trigger)
{
    if ((g_cursor % 3u) != 2u)
    {
        g_cursor++;
    }
    g_notice = 0;
    return trigger;
}

static UI_16 TttPlaceXAction(UI_16 trigger)
{
    (void)TttPlaceMark(TTT_MARK_X, g_cursor);
    return trigger;
}

static UI_16 TttPlaceOAction(UI_16 trigger)
{
    (void)TttPlaceMark(TTT_MARK_O, g_cursor);
    return trigger;
}

static UI_16 TttNewGameAction(UI_16 trigger)
{
    TttResetBoard();
    return trigger;
}

static UI_16 TttStart2PAction(UI_16 trigger)
{
    g_gameMode = TTT_MODE_2P;
    TttResetBoard();
    return trigger;
}

static UI_16 TttStartCpuAction(UI_16 trigger)
{
    g_gameMode = TTT_MODE_CPU;
    TttResetBoard();
    return trigger;
}

static UI_16 TttBackAction(UI_16 trigger)
{
    g_menuSel = (g_gameMode == TTT_MODE_CPU) ? TTT_MENU_CPU : TTT_MENU_2P;
    g_notice = 0;
    return trigger;
}

/*=========================================================
 * 状态迁移表与状态对象
 *
 * 每个状态表后紧跟对应的 StateType_t，便于对照阅读。
 * 自动迁移状态是过渡态，不对应实际画面，screenId 使用 SCREEN_ID_NONE。
 *=========================================================*/

/* 主菜单 */
static const StateTable_t TttMenuRules[] =
{
    { KEY_UP,    TttMenuUpAction,   &StateTttMenu },
    { KEY_DOWN,  TttMenuDownAction, &StateTttMenu },
    { KEY_ENTER, TttNoop,         &StateTttMenuDecide },
};

PUBLIC const StateType_t StateTttMenu =
{
    TttNoop,
    NO,
    STATE_RULES(TttMenuRules),
    SCREEN_TTT_MENU
};

/* 菜单确认：把选中项翻译成启动/退出触发器 */
static const StateTable_t TttMenuDecideRules[] =
{
    { TRIG_TTT_START_2P,  TttStart2PAction,  &StateTttXTurn },
    { TRIG_TTT_START_CPU, TttStartCpuAction, &StateTttXTurn },
    { TRIG_TTT_QUIT,      TttNoop,         &StateTttQuit },
};

PUBLIC const StateType_t StateTttMenuDecide =
{
    TttMenuDecideEntry,
    YES,
    STATE_RULES(TttMenuDecideRules),
    SCREEN_ID_NONE
};

/* X 回合 */
static const StateTable_t TttXTurnRules[] =
{
    { KEY_UP,    TttMoveUpAction,    &StateTttXTurn },
    { KEY_DOWN,  TttMoveDownAction,  &StateTttXTurn },
    { KEY_LEFT,  TttMoveLeftAction,  &StateTttXTurn },
    { KEY_RIGHT, TttMoveRightAction, &StateTttXTurn },
    { KEY_ENTER, TttPlaceXAction,    &StateTttCheck },
    { KEY_ESC,   TttBackAction,      &StateTttMenu },
};

PUBLIC const StateType_t StateTttXTurn =
{
    TttXTurnEntry,
    NO,
    STATE_RULES(TttXTurnRules),
    SCREEN_TTT_XTURN
};

/* O 回合 */
static const StateTable_t TttOTurnRules[] =
{
    { KEY_UP,    TttMoveUpAction,    &StateTttOTurn },
    { KEY_DOWN,  TttMoveDownAction,  &StateTttOTurn },
    { KEY_LEFT,  TttMoveLeftAction,  &StateTttOTurn },
    { KEY_RIGHT, TttMoveRightAction, &StateTttOTurn },
    { KEY_ENTER, TttPlaceOAction,    &StateTttCheck },
    { KEY_ESC,   TttBackAction,      &StateTttMenu },
};

PUBLIC const StateType_t StateTttOTurn =
{
    TttOTurnEntry,
    NO,
    STATE_RULES(TttOTurnRules),
    SCREEN_TTT_OTURN
};

/* 胜负检查：入口函数返回的触发器驱动引擎继续自动迁移 */
static const StateTable_t TttCheckRules[] =
{
    { TRIG_TTT_TO_X,   TttNoop, &StateTttXTurn },
    { TRIG_TTT_TO_O,   TttNoop, &StateTttOTurn },
    { TRIG_TTT_TO_CPU, TttNoop, &StateTttComputerTurn },
    { TRIG_TTT_X_WIN,  TttNoop, &StateTttXWin },
    { TRIG_TTT_O_WIN,  TttNoop, &StateTttOWin },
    { TRIG_TTT_DRAW,   TttNoop, &StateTttDraw },
};

PUBLIC const StateType_t StateTttCheck =
{
    TttCheckEntry,
    YES,
    STATE_RULES(TttCheckRules),
    SCREEN_ID_NONE
};

/* 电脑回合：落子后回到 CHECK 继续判定 */
static const StateTable_t TttComputerTurnRules[] =
{
    { TRIG_TTT_COMPUTER_DONE, TttNoop, &StateTttCheck },
    { TRIG_TTT_DRAW,          TttNoop, &StateTttDraw },
};

PUBLIC const StateType_t StateTttComputerTurn =
{
    TttComputerTurnEntry,
    YES,
    STATE_RULES(TttComputerTurnRules),
    SCREEN_ID_NONE
};

/* X 获胜 */
static const StateTable_t TttXWinRules[] =
{
    { KEY_UP,    TttResultUpAction,   &StateTttXWin },
    { KEY_DOWN,  TttResultDownAction, &StateTttXWin },
    { KEY_ENTER, TttNoop,           &StateTttResultDecide },
    { KEY_ESC,   TttBackAction,       &StateTttMenu },
};

PUBLIC const StateType_t StateTttXWin =
{
    TttNoop,
    NO,
    STATE_RULES(TttXWinRules),
    SCREEN_TTT_XWIN
};

/* O 获胜 */
static const StateTable_t TttOWinRules[] =
{
    { KEY_UP,    TttResultUpAction,   &StateTttOWin },
    { KEY_DOWN,  TttResultDownAction, &StateTttOWin },
    { KEY_ENTER, TttNoop,           &StateTttResultDecide },
    { KEY_ESC,   TttBackAction,       &StateTttMenu },
};

PUBLIC const StateType_t StateTttOWin =
{
    TttNoop,
    NO,
    STATE_RULES(TttOWinRules),
    SCREEN_TTT_OWIN
};

/* 平局 */
static const StateTable_t TttDrawRules[] =
{
    { KEY_UP,    TttResultUpAction,   &StateTttDraw },
    { KEY_DOWN,  TttResultDownAction, &StateTttDraw },
    { KEY_ENTER, TttNoop,           &StateTttResultDecide },
    { KEY_ESC,   TttBackAction,       &StateTttMenu },
};

PUBLIC const StateType_t StateTttDraw =
{
    TttNoop,
    NO,
    STATE_RULES(TttDrawRules),
    SCREEN_TTT_DRAW
};

/* 结果确认：把选中项翻译成再开一局/返回菜单触发器 */
static const StateTable_t TttResultDecideRules[] =
{
    { TRIG_TTT_REPLAY, TttNewGameAction, &StateTttXTurn },
    { TRIG_TTT_MENU,   TttBackAction,    &StateTttMenu },
};

PUBLIC const StateType_t StateTttResultDecide =
{
    TttResultDecideEntry,
    YES,
    STATE_RULES(TttResultDecideRules),
    SCREEN_ID_NONE
};

/* 退出状态：不再处理按键，主循环渲染后直接结束 */
static const StateTable_t TttQuitRules[] =
{
    { KEY_ESC, TttNoop, &StateTttQuit },
};

PUBLIC const StateType_t StateTttQuit =
{
    TttNoop,
    NO,
    STATE_RULES(TttQuitRules),
    SCREEN_TTT_QUIT
};

/*=========================================================
 * 屏幕层只读访问接口
 *=========================================================*/

char TttGetCell(UI_8 idx)
{
    return g_board[idx];
}

UI_8 TttGetCursor(void)
{
    return g_cursor;
}

char TttGetCurrentMover(void)
{
    return g_currentMover;
}

UI_8 TttGetMenuSel(void)
{
    return g_menuSel;
}

UI_8 TttGetResultSel(void)
{
    return g_resultSel;
}

UI_8 TttGetMode(void)
{
    return g_gameMode;
}

const char *TttGetNotice(void)
{
    return g_notice;
}
