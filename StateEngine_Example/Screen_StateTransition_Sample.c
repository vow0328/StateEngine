#include <stdio.h>
#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#endif

#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

typedef struct ScreenContext
{
    const StateType_t *previousState;
    const char *pendingName;
    int inputIsConsole;
    int outputIsConsole;
    int ansiEnabled;
#ifdef _WIN32
    HANDLE output;
    DWORD originalMode;
    UINT originalCodePage;
#endif
} ScreenContext;

static ScreenContext g_screen;

void ScreenInit(void)
{
#ifdef _WIN32
    g_screen.inputIsConsole = _isatty(_fileno(stdin));
    g_screen.output = GetStdHandle(STD_OUTPUT_HANDLE);
    g_screen.outputIsConsole = GetConsoleMode(g_screen.output, &g_screen.originalMode) != 0;
    if (g_screen.outputIsConsole)
    {
        g_screen.originalCodePage = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
        g_screen.ansiEnabled = SetConsoleMode(g_screen.output,
            g_screen.originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
#endif
}

void ScreenShutdown(void)
{
    printf("\n  已退出演示，模拟数据未写入磁盘。\n");
    fflush(stdout);
#ifdef _WIN32
    if (g_screen.outputIsConsole)
    {
        SetConsoleMode(g_screen.output, g_screen.originalMode);
        SetConsoleOutputCP(g_screen.originalCodePage);
    }
#endif
}

static void ScreenClear(void)
{
#ifdef _WIN32
    if (g_screen.outputIsConsole)
    {
        if (g_screen.ansiEnabled)
        {
            printf("\033[2J\033[H");
        }
        else
        {
            CONSOLE_SCREEN_BUFFER_INFO info;
            DWORD written;
            COORD origin = { 0, 0 };

            if (GetConsoleScreenBufferInfo(g_screen.output, &info))
            {
                DWORD cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
                FillConsoleOutputCharacterA(g_screen.output, ' ', cells, origin, &written);
                FillConsoleOutputAttribute(g_screen.output, info.wAttributes, cells, origin, &written);
                SetConsoleCursorPosition(g_screen.output, origin);
            }
        }
    }
#endif
}

static void ScreenAccent(const char *text)
{
    if (g_screen.ansiEnabled)
    {
        printf("\033[1;36m%s\033[0m", text);
    }
    else
    {
        printf("%s", text);
    }
}

static UI_16 ScreenMapKey(int key)
{
    switch (key)
    {
    case 'w':
    case 'W':
        return KEY_UP;
    case 's':
    case 'S':
        return KEY_DOWN;
    case 'd':
    case 'D':
        return KEY_DELETE;
    case 'c':
    case 'C':
    case 27:
        return KEY_ESC;
    case '\n':
    case '\r':
        return KEY_ENTER;
    case 'q':
    case 'Q':
        return SCREEN_KEY_QUIT;
    default:
        return SCREEN_KEY_NONE;
    }
}

UI_16 ScreenReadTrigger(void)
{
#ifdef _WIN32
    if (g_screen.inputIsConsole)
    {
        int key = _getch();

        if ((key == 0) || (key == 224))
        {
            key = _getch();
            if (key == 72) return KEY_UP;
            if (key == 80) return KEY_DOWN;
            if (key == 83) return KEY_DELETE;
            return SCREEN_KEY_NONE;
        }
        return ScreenMapKey(key);
    }
#endif

    {
        char line[16];

        if (fgets(line, (int)sizeof(line), stdin) == NULL)
        {
            return SCREEN_KEY_QUIT;
        }
        return ScreenMapKey((unsigned char)line[0]);
    }
}

void ScreenWaitForWork(void)
{
#ifdef _WIN32
    if (g_screen.inputIsConsole && g_screen.outputIsConsole)
    {
        Sleep(650);
    }
#endif
}

void ScreenRenderCurrent(void)
{
    const DatabaseContext *database = DbGetContext();
    const StateType_t *currentState = STATE_CURRENT();
    UI_8 index;

    ScreenClear();
    printf("\n  ");
    ScreenAccent("DATABASE WORKSPACE");
    printf("  /  数据库管理演示                      [模拟数据]\n");
    printf("  ------------------------------------------------------------\n");

    if ((g_screen.previousState == &StateDeleteWorking) && (g_screen.pendingName != 0))
    {
        printf("\n  [完成] 已删除 %s，仅影响本次内存数据。\n", g_screen.pendingName);
        g_screen.pendingName = 0;
    }

    switch (currentState->screenId)
    {
    case SCREEN_HOME:
        printf("\n  01  首页\n\n");
        printf("  管理模拟数据库，观察不同层次的状态迁移。\n");
        printf("  当前可用：%u 个数据库\n\n", (unsigned int)database->count);
        printf("  [进入选择页]  先自动检查数据，再按需初始化页面。\n");
        printf("\n  操作  Enter 进入  |  Q 退出\n");
        break;

    case SCREEN_DATABASE_SELECT:
        printf("\n  02  选择数据库                         %u / %u\n\n",
               (unsigned int)(database->selected + 1u), (unsigned int)database->count);
        for (index = 0u; index < database->count; index++)
        {
            printf("    %s  %02u   %s%s\n", (index == database->selected) ? ">" : " ",
                   (unsigned int)(index + 1u), database->items[index],
                   (index == database->selected) ? "   [当前选择]" : "");
        }
        if (g_screen.previousState == &StateDatabaseSelect)
        {
            printf("\n  页内操作：仅更新选择并重新进入页面，不重复初始化。\n");
        }
        else if (g_screen.previousState == &StateDeleteConfirm)
        {
            printf("\n  已取消删除，保留当前选择。\n");
        }
        else
        {
            printf("\n  页内移动不会经过入口检查或重复初始化。\n");
        }
        if (g_screen.inputIsConsole)
        {
            printf("\n  操作  上/下 或 W/S 选择  |  D/Delete 删除  |  Esc/C 返回\n");
        }
        else
        {
            printf("\n  操作  W/S 选择  |  D 删除  |  C 返回\n");
        }
        break;

    case SCREEN_DATABASE_EMPTY:
        printf("\n  02  暂无数据库\n\n");
        printf("  入口或删除后的检查发现列表为空。\n");
        printf("  返回首页后再次进入，仍会经过入口检查。\n");
        printf("\n  操作  %s 返回首页\n", g_screen.inputIsConsole ? "Esc/C" : "C");
        break;

    case SCREEN_DELETE_CONFIRM:
        printf("\n  03  确认删除\n\n");
        printf("  目标：%s\n", database->items[database->selected]);
        printf("  此操作仅删除模拟数据，不会修改真实数据库或文件。\n");
        printf("\n  操作  Enter 确认删除  |  %s 取消并保留选择\n",
               g_screen.inputIsConsole ? "Esc/C" : "C");
        break;

    case SCREEN_DELETE_WORKING:
        g_screen.pendingName = database->items[database->selected];
        printf("\n  04  正在处理\n\n");
        printf("  正在模拟删除 %s ...\n", g_screen.pendingName);
        printf("  工作状态先返回主循环，随后由模拟事件继续执行。\n");
        break;

    default:
        printf("\n  无法显示画面：%u\n", (unsigned int)currentState->screenId);
        printf("  操作  Q 退出\n");
        break;
    }

    printf("\n  ------------------------------------------------------------\n");
    printf("  INIT %-5u 页面初始化  |  DISPLAY %-5u 状态进入\n",
           (unsigned int)database->initCount, (unsigned int)database->displayCount);
    printf("  全局  Q 退出\n");
    if (!g_screen.inputIsConsole)
    {
        printf("  行输入模式：输入字符后按 Enter；确认操作直接按 Enter。\n");
    }
    fflush(stdout);
    g_screen.previousState = currentState;
}
