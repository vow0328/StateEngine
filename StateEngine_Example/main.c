#include <stdio.h>
#include "MainStat.h"
#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

/*=========================================================
 * 交互式井字棋主循环
 *
 * 读键 -> 交给状态引擎迁移 -> 引擎返回后按 screenId 渲染。
 * 引擎负责状态表和状态栈；Screen 层只负责输入与显示。
 *=========================================================*/

int main(void)
{
    UI_16 key;

    TttScreenInit();
    StateEngineInit(&StateTttMenu);
    TttRenderCurrentScreen();

    for (;;)
    {
        key = TttScreenReadTrigger();

        if (key == SCREEN_KEY_QUIT)
        {
            printf("\n退出。\n");
            break;
        }

        if (key == SCREEN_KEY_NONE)
        {
            continue;
        }

        StateEngine(key);
        TttRenderCurrentScreen();

        /* 菜单选择“退出”后进入终止状态 */
        if (STATE_CURRENT() == &StateTttQuit)
        {
            break;
        }
    }

    return 0;
}
