#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"

static DatabaseContext g_database = {
    { "Products", "Orders", "Customers" }, 3u, 0u, 0u, 0u
};

static UI_16 DummyFunc(UI_16 trigger);
static UI_16 DisplayScreen(UI_16 trigger);
static UI_16 CheckDatabaseSelect(UI_16 trigger);
static UI_16 InitDatabaseSelect(UI_16 trigger);
static UI_16 MoveDatabasePrevious(UI_16 trigger);
static UI_16 MoveDatabaseNext(UI_16 trigger);
static UI_16 DeleteDatabaseContent(UI_16 trigger);
static UI_16 CheckAfterDelete(UI_16 trigger);

static const StateTable_t HomeRules[] = {
    { EVENT_HOME_READY, DummyFunc, &StateHome },
    { KEY_ENTER, DummyFunc, &StateDatabaseEntry }
};

const StateType_t StateHome = {
    DisplayScreen, NO, STATE_RULES(HomeRules), SCREEN_HOME
};

/* 进入选择页前检查：只在跨页面进入时经过，不在页内移动时重复执行。 */
static const StateTable_t DatabaseEntryRules[] = {
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty },
    { EVENT_DB_OK, InitDatabaseSelect, &StateDatabaseSelect }
};

const StateType_t StateDatabaseEntry = {
    CheckDatabaseSelect, YES, STATE_RULES(DatabaseEntryRules), SCREEN_ID_NONE
};

/* 同页操作回到本状态，仅执行对应动作和画面前处理，不走入口检查。 */
static const StateTable_t DatabaseSelectRules[] = {
    { KEY_UP, MoveDatabasePrevious, &StateDatabaseSelect },
    { KEY_DOWN, MoveDatabaseNext, &StateDatabaseSelect },
    { KEY_DELETE, DummyFunc, &StateDeleteConfirm },
    { KEY_ESC, DummyFunc, &PreviousState }
};

const StateType_t StateDatabaseSelect = {
    DisplayScreen, NO, STATE_RULES(DatabaseSelectRules), SCREEN_DATABASE_SELECT
};

static const StateTable_t DatabaseEmptyRules[] = {
    { KEY_ESC, DummyFunc, &StateHome }
};

const StateType_t StateDatabaseEmpty = {
    DisplayScreen, NO, STATE_RULES(DatabaseEmptyRules), SCREEN_DATABASE_EMPTY
};

static const StateTable_t DeleteConfirmRules[] = {
    { KEY_ESC, DummyFunc, &StateDatabaseSelect },
    { KEY_ENTER, DummyFunc, &StateDeleteWorking }
};

const StateType_t StateDeleteConfirm = {
    DisplayScreen, NO, STATE_RULES(DeleteConfirmRules), SCREEN_DELETE_CONFIRM
};

/* 工作状态为 NO：先返回主循环显示工作画面，再等待执行事件。 */
static const StateTable_t DeleteWorkingRules[] = {
    { EVENT_DELETE_EXECUTE, DeleteDatabaseContent, &StateDeleteCheck }
};

const StateType_t StateDeleteWorking = {
    DisplayScreen, NO, STATE_RULES(DeleteWorkingRules), SCREEN_DELETE_WORKING
};

/* 删除后的检查根据剩余数量自动返回选择页或进入空数据页。 */
static const StateTable_t DeleteCheckRules[] = {
    { EVENT_DB_OK, InitDatabaseSelect, &StateDatabaseSelect },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty }
};

const StateType_t StateDeleteCheck = {
    CheckAfterDelete, YES, STATE_RULES(DeleteCheckRules), SCREEN_ID_NONE
};

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
    return (g_database.count > 0u) ? EVENT_DB_OK : EVENT_DB_EMPTY;
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
    return CheckDatabaseSelect(trigger);
}

const DatabaseContext *DbGetContext(void)
{
    return &g_database;
}
