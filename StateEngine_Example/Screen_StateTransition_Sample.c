#include <stdio.h>
#include <stdlib.h>
#include "MainStat.h"
#include "Screen_StateTransition_Sample.h"
#include "Action_StateTransition_Sample.h"
#include "TriggerDefine_Sample.h"

#ifdef _WIN32
#include <conio.h>
#endif

/*=========================================================
 * 屏幕层：控制台初始化、按键读取、清屏、画面绘制
 *=========================================================*/

void TttScreenInit(void)
{
#ifdef _WIN32
    /* 源码与字符串都是 UTF-8；把控制台代码页切到 65001，
       否则 Windows 控制台会按 GBK 解码，中文显示为乱码。 */
    (void)system("chcp 65001 >nul");
#endif
}

static void ScreenClear(void)
{
#ifdef _WIN32
    (void)system("cls");
#else
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

UI_16 TttScreenReadTrigger(void)
{
#ifdef _WIN32
    int ch;
    int code;

    ch = _getch();

    /* 方向键：部分终端返回 0，部分返回 224 */
    if ((ch == 0) || (ch == 224))
    {
        code = _getch();
        switch (code)
        {
        case 72:
            return KC_UP;
        case 80:
            return KC_DOWN;
        case 75:
            return KC_LEFT;
        case 77:
            return KC_RIGHT;
        default:
            return SCREEN_KEY_NONE;
        }
    }

    switch (ch)
    {
    case 'w':
    case 'W':
        return KC_UP;

    case 's':
    case 'S':
        return KC_DOWN;

    case 'a':
    case 'A':
        return KC_LEFT;

    case 'd':
    case 'D':
        return KC_RIGHT;

    case ' ':
    case 10:
    case 13:
        return KC_ENTER;

    case 27:
        return KC_ESC;

    case 'q':
    case 'Q':
        return SCREEN_KEY_QUIT;

    default:
        return SCREEN_KEY_NONE;
    }
#else
    /* 非 Windows：按行输入，每行一个操作；空行表示 Enter */
    char line[16];

    if (fgets(line, (int)sizeof(line), stdin) == NULL)
    {
        return SCREEN_KEY_QUIT;
    }

    switch (line[0])
    {
    case 'w':
    case 'W':
        return KC_UP;

    case 's':
    case 'S':
        return KC_DOWN;

    case 'a':
    case 'A':
        return KC_LEFT;

    case 'd':
    case 'D':
        return KC_RIGHT;

    case ' ':
    case '\n':
    case '\r':
        return KC_ENTER;

    case 27:
        return KC_ESC;

    case 'q':
    case 'Q':
        return SCREEN_KEY_QUIT;

    default:
        return SCREEN_KEY_NONE;
    }
#endif
}

/*=========================================================
 * 画面绘制
 *=========================================================*/

static void TttPrintCell(UI_8 idx, BOOL showCursor)
{
    char c;

    c = TttGetCell(idx);
    if (c == (char)0)
    {
        /* 空位不显示数字，避免和 X/O 混淆；光标仍用 [] 标出 */
        c = ' ';
    }

    if ((showCursor != FALSE) && (idx == TttGetCursor()))
    {
        printf("[%c]", c);
    }
    else
    {
        printf(" %c ", c);
    }
}

static void TttPrintBoard(BOOL showCursor)
{
    UI_8 row;
    UI_8 col;

    for (row = 0u; row < 3u; row++)
    {
        printf("  +---+---+---+\n");
        printf("  |");
        for (col = 0u; col < 3u; col++)
        {
            TttPrintCell((UI_8)(row * 3u + col), showCursor);
            printf("|");
        }
        printf("\n");
    }
    printf("  +---+---+---+\n");
}

static void TttRenderMenu(void)
{
    UI_8 sel;

    sel = TttGetMenuSel();
    printf("  主菜单\n\n");
    printf("  %s 双人对战\n", (sel == TTT_MENU_2P) ? ">" : " ");
    printf("  %s 人机对战（你执 X）\n", (sel == TTT_MENU_CPU) ? ">" : " ");
    printf("  %s 退出\n", (sel == TTT_MENU_QUIT) ? ">" : " ");
    printf("\n  ↑↓/W/S 选择   Enter 确认   Q 退出\n");
}

static void TttRenderGame(void)
{
    const char *notice;

    printf("  %s\n\n", (TttGetMode() == TTT_MODE_CPU) ? "人机对战（你执 X）" : "双人对战");
    printf("  %c 回合\n\n", TttGetCurrentMover());
    TttPrintBoard(TRUE);
    printf("\n  ↑↓←→/WASD 移动   Enter 落子   Esc 返回菜单   Q 退出\n");

    notice = TttGetNotice();
    if (notice != NULL)
    {
        printf("\n  %s\n", notice);
    }
}

static void TttRenderResult(const char *msg)
{
    UI_8 sel;

    sel = TttGetResultSel();
    printf("  %s\n\n", msg);
    TttPrintBoard(FALSE);
    printf("\n  %s 再来一局\n", (sel == TTT_RESULT_REPLAY) ? ">" : " ");
    printf("  %s 返回主菜单\n", (sel == TTT_RESULT_MENU) ? ">" : " ");
    printf("\n  ↑↓/W/S 选择   Enter 确认   Esc 返回菜单\n");
}

void TttRenderCurrentScreen(void)
{
    ScreenClear();
    printf("=== 井字棋 ===\n\n");

    /* 自动迁移状态在引擎内部执行完，不会成为最终状态；
       这里只处理实际画面 ID。 */
    switch (GetStateTbl()->scrnId)
    {
    case SCRN_TTT_MENU:
        TttRenderMenu();
        break;

    case SCRN_TTT_XTURN:
    case SCRN_TTT_OTURN:
        TttRenderGame();
        break;

    case SCRN_TTT_XWIN:
        TttRenderResult((TttGetMode() == TTT_MODE_CPU) ? "你赢了！" : "X 获胜！");
        break;

    case SCRN_TTT_OWIN:
        TttRenderResult((TttGetMode() == TTT_MODE_CPU) ? "电脑获胜！" : "O 获胜！");
        break;

    case SCRN_TTT_DRAW:
        TttRenderResult("平局。");
        break;

    case SCRN_TTT_QUIT:
        printf("  谢谢游玩。\n");
        break;

    default:
        printf("  [未知画面 %u]\n", (unsigned int)GetStateTbl()->scrnId);
        break;
    }

    fflush(stdout);
}
