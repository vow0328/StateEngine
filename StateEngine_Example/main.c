#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

int main(void)
{
    UI_16 trigger;

    ScreenInit();
    StateEngineInit(&StateHome);
    /* 初始化不入历史栈，自迁移一次以便选择页可以回退首页。 */
    StateEngine(EVENT_HOME_READY);
    ScreenRenderCurrent();

    for (;;)
    {
        if (STATE_CURRENT() == &StateDeleteWorking)
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
