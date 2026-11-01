#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

static DatabaseContext g_database = {
    { "Products", "Orders", "Customers" }, 3u, 0u, 0u, 0u, NO
};

static UI_16 DummyFunc(UI_16 trigger);
static UI_16 DisplayScreen(UI_16 trigger);
static UI_16 CheckDatabaseSelect(UI_16 trigger);
static UI_16 InitDatabaseSelect(UI_16 trigger);
static UI_16 MoveDatabasePrevious(UI_16 trigger);
static UI_16 MoveDatabaseNext(UI_16 trigger);
static UI_16 DeleteDatabaseContent(UI_16 trigger);
static UI_16 CheckAfterDelete(UI_16 trigger);
static UI_16 ToggleEntryError(UI_16 trigger);

/*========== 首页 ==========*/
static const StateTable_t HomeRules[] = {
    { EVENT_HOME_READY, DummyFunc, &StateHome },
    { KEY_ENTER, DummyFunc, &StateDatabaseEntry },
    { KEY_ERROR, ToggleEntryError, &StateHome }
};

const StateType_t StateHome = {
    DisplayScreen, NO, STATE_RULES(HomeRules), SCREEN_HOME
};

/*========== 选择页入口检查：跨页面进入时自动执行 ==========*/
static const StateTable_t DatabaseEntryRules[] = {
    { EVENT_DB_ERROR, DummyFunc, &StateDatabaseError },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty },
    { EVENT_DB_OK, InitDatabaseSelect, &StateDatabaseSelect }
};

const StateType_t StateDatabaseEntry = {
    CheckDatabaseSelect, YES, STATE_RULES(DatabaseEntryRules), SCREEN_ID_NONE
};

/*========== 数据库选择：页内操作不经过入口检查 ==========*/
static const StateTable_t DatabaseSelectRules[] = {
    { KEY_UP, MoveDatabasePrevious, &StateDatabaseSelect },
    { KEY_DOWN, MoveDatabaseNext, &StateDatabaseSelect },
    { KEY_DELETE, DummyFunc, &StateDeleteConfirm },
    { KEY_ESC, DummyFunc, &PreviousState }
};

const StateType_t StateDatabaseSelect = {
    DisplayScreen, NO, STATE_RULES(DatabaseSelectRules), SCREEN_DATABASE_SELECT
};

/*========== 空数据提示 ==========*/
static const StateTable_t DatabaseEmptyRules[] = {
    { KEY_ESC, DummyFunc, &StateHome }
};

const StateType_t StateDatabaseEmpty = {
    DisplayScreen, NO, STATE_RULES(DatabaseEmptyRules), SCREEN_DATABASE_EMPTY
};

/*========== 入口检查错误提示 ==========*/
static const StateTable_t DatabaseErrorRules[] = {
    { KEY_ESC, DummyFunc, &StateHome }
};

const StateType_t StateDatabaseError = {
    DisplayScreen, NO, STATE_RULES(DatabaseErrorRules), SCREEN_DATABASE_ERROR
};

/*========== 删除确认 ==========*/
static const StateTable_t DeleteConfirmRules[] = {
    { KEY_ESC, DummyFunc, &StateDatabaseSelect },
    { KEY_ENTER, DummyFunc, &StateDeleteWorking }
};

const StateType_t StateDeleteConfirm = {
    DisplayScreen, NO, STATE_RULES(DeleteConfirmRules), SCREEN_DELETE_CONFIRM
};

/*========== 删除工作：先显示工作画面，再等待执行事件 ==========*/
static const StateTable_t DeleteWorkingRules[] = {
    { EVENT_DELETE_EXECUTE, DeleteDatabaseContent, &StateDeleteCheck }
};

const StateType_t StateDeleteWorking = {
    DisplayScreen, NO, STATE_RULES(DeleteWorkingRules), SCREEN_DELETE_WORKING
};

/*========== 删除结果检查：自动进入选择页或空数据页 ==========*/
static const StateTable_t DeleteCheckRules[] = {
    { EVENT_DB_OK, InitDatabaseSelect, &StateDatabaseSelect },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty }
};

const StateType_t StateDeleteCheck = {
    CheckAfterDelete, YES, STATE_RULES(DeleteCheckRules), SCREEN_ID_NONE
};

/* 无操作占位函数：不做业务处理，原样传递触发值。 */
static UI_16 DummyFunc(UI_16 trigger)
{
    return trigger;
}

static UI_16 DisplayScreen(UI_16 trigger)
{
    g_database.displayCount++;
    return trigger;
}

static UI_16 CheckDatabaseSelect(UI_16 trigger)
{
    (void)trigger;
    if (g_database.simulateEntryError != NO)
    {
        return EVENT_DB_ERROR;
    }
    return (g_database.count > 0u) ? EVENT_DB_OK : EVENT_DB_EMPTY;
}

static UI_16 ToggleEntryError(UI_16 trigger)
{
    g_database.simulateEntryError = (g_database.simulateEntryError == NO) ? YES : NO;
    return trigger;
}

static UI_16 InitDatabaseSelect(UI_16 trigger)
{
    g_database.selected = 0u;
    g_database.initCount++;
    return trigger;
}

static UI_16 MoveDatabasePrevious(UI_16 trigger)
{
    if (g_database.selected > 0u)
    {
        g_database.selected--;
    }
    return trigger;
}

static UI_16 MoveDatabaseNext(UI_16 trigger)
{
    if ((UI_8)(g_database.selected + 1u) < g_database.count)
    {
        g_database.selected++;
    }
    return trigger;
}

static UI_16 DeleteDatabaseContent(UI_16 trigger)
{
    UI_8 index;

    if ((g_database.count > 0u) && (g_database.selected < g_database.count))
    {
        for (index = g_database.selected; (UI_8)(index + 1u) < g_database.count; index++)
        {
            g_database.items[index] = g_database.items[index + 1u];
        }
        g_database.count--;
        g_database.items[g_database.count] = 0;
    }
    return trigger;
}

static UI_16 CheckAfterDelete(UI_16 trigger)
{
    (void)trigger;
    return (g_database.count > 0u) ? EVENT_DB_OK : EVENT_DB_EMPTY;
}

const DatabaseContext *DbGetContext(void)
{
    return &g_database;
}
