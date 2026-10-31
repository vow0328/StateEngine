#ifndef ACTION_STATE_TRANSITION_SAMPLE_H
#define ACTION_STATE_TRANSITION_SAMPLE_H

#include "MainStat.h"
#include "TriggerDefine_Sample.h"

#define EVENT_HOME_READY  ((UI_16)0x0400u)
#define EVENT_DELETE_EXECUTE ((UI_16)0x0401u)
#define EVENT_DB_OK       ((UI_16)0x0402u)
#define EVENT_DB_EMPTY    ((UI_16)0x0403u)

typedef struct DatabaseContext
{
    const char *items[3];
    UI_8 count;
    UI_8 selected;
    UI_16 initCount;
    UI_16 displayCount;
} DatabaseContext;

extern const StateType_t PreviousState;
extern const StateType_t StateHome;
extern const StateType_t StateDatabaseEntry;
extern const StateType_t StateDatabaseSelect;
extern const StateType_t StateDatabaseEmpty;
extern const StateType_t StateDeleteConfirm;
extern const StateType_t StateDeleteWorking;
extern const StateType_t StateDeleteCheck;

const DatabaseContext *DbGetContext(void);

#endif /* ACTION_STATE_TRANSITION_SAMPLE_H */
