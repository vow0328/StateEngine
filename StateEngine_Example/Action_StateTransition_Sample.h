#ifndef ACTION_STATE_TRANSITION_SAMPLE_H
#define ACTION_STATE_TRANSITION_SAMPLE_H

#include "MainStat.h"
#include "TriggerDefine_Sample.h"

#define EVENT_HOME_READY  ((UI_16)0x0400u)
#define EVENT_DELETE_EXECUTE ((UI_16)0x0401u)
#define EVENT_DB_OK       ((UI_16)0x0402u)
#define EVENT_DB_EMPTY    ((UI_16)0x0403u)
#define EVENT_DB_ERROR    ((UI_16)0x0404u)
#define DATABASE_CELL_CAPACITY 24u
#define DATABASE_ROWS 2u
#define DATABASE_COLUMNS 2u

typedef struct DatabaseItem
{
    const char *name;
    char cells[DATABASE_ROWS][DATABASE_COLUMNS][DATABASE_CELL_CAPACITY];
} DatabaseItem;

typedef struct DatabaseContext
{
    DatabaseItem *items[3];
    UI_8 count;
    UI_8 selected;
    UI_8 row;
    UI_8 column;
    BOOL simulateEntryError;
    char editBackup[DATABASE_CELL_CAPACITY];
} DatabaseContext;

extern const StateType_t PreviousState;
extern const StateType_t FormEntryState;
extern const StateType_t StateHome;
extern const StateType_t StateDatabaseEntry;
extern const StateType_t StateDatabaseSelect;
extern const StateType_t StateDatabaseContent;
extern const StateType_t StateDatabaseEdit;
extern const StateType_t StateDatabaseEmpty;
extern const StateType_t StateDatabaseError;
extern const StateType_t StateDeleteConfirm;
extern const StateType_t StateDeleteWorking;
extern const StateType_t StateDeleteCheck;

const DatabaseContext *DbGetContext(void);

#endif /* ACTION_STATE_TRANSITION_SAMPLE_H */
