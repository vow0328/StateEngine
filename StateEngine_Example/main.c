#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

int main(void)
{
    UI_16 trigger;

    ScreenInit();
    StateEngineInit(&StateHome);
    /* 初始化不入历史；准备事件经首页自动入口过渡后保存首页状态表指针。 */
    StateEngine(EVENT_HOME_READY);
    ScreenRenderCurrent();

    for (;;)
    {
        if (GetStatePtr() == &StateDeleteWorking)
        {
            ScreenWaitForWork();
            /* 示例模拟下一轮执行事件，实机应由调度器或设备产生。 */
            trigger = EVENT_DELETE_EXECUTE;
        }
        else
        {
            trigger = ScreenReadTrigger();
            if (trigger == SCREEN_KEY_QUIT)
            {
                break;
            }
            if (trigger == SCREEN_KEY_NONE)
            {
                continue;
            }
        }

        StateEngine(trigger);
        ScreenRenderCurrent();
    }

    ScreenShutdown();
    return 0;
}
