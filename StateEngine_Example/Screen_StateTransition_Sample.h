#ifndef SCREEN_STATE_TRANSITION_SAMPLE_H
#define SCREEN_STATE_TRANSITION_SAMPLE_H

#include "TypeDefine_Sample.h"

#define SCREEN_HOME           100u
#define SCREEN_DATABASE_SELECT 101u
#define SCREEN_DATABASE_EMPTY  102u
#define SCREEN_DELETE_CONFIRM  103u
#define SCREEN_DELETE_WORKING  104u

#define SCREEN_KEY_NONE ((UI_16)0x0000u)
#define SCREEN_KEY_QUIT ((UI_16)0xFFFFu)

UI_16 ScreenReadTrigger(void);
void ScreenInit(void);
void ScreenShutdown(void);
void ScreenWaitForWork(void);
void ScreenRenderCurrent(void);

#endif /* SCREEN_STATE_TRANSITION_SAMPLE_H */
