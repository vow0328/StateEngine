#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"
#include <string.h>

static DatabaseItem g_databaseItems[] = {
    { "Products", {{ "P001", "Keyboard" }, { "P002", "Mouse" }} },
    { "Orders", {{ "O001", "Pending" }, { "O002", "Completed" }} },
    { "Customers", {{ "C001", "Alice" }, { "C002", "Bob" }} }
};

static DatabaseContext g_database = {
    { &g_databaseItems[0], &g_databaseItems[1], &g_databaseItems[2] }, 3u, 0u, 0u, 0u, NO, { 0 }
};

static UI_16 DummyFunc(UI_16 trigger);
static UI_16 CheckDatabaseSelect(UI_16 trigger);
static UI_16 InitDatabaseSelect(UI_16 trigger);
static UI_16 MoveDatabasePrevious(UI_16 trigger);
static UI_16 MoveDatabaseNext(UI_16 trigger);
static UI_16 DeleteDatabaseContent(UI_16 trigger);
static UI_16 CheckAfterDelete(UI_16 trigger);
static UI_16 ToggleEntryError(UI_16 trigger);
static UI_16 SelectHomeFromHistory(UI_16 trigger);
static UI_16 BeginEdit(UI_16 trigger);
static UI_16 AppendCharacter(UI_16 trigger);
static UI_16 EraseCharacter(UI_16 trigger);
static UI_16 CancelEdit(UI_16 trigger);
static UI_16 OpenDatabase(UI_16 trigger);
static UI_16 MoveCell(UI_16 trigger);
static const StateType_t StateDeleteResume;

/*========== 首页 ==========*/
static const StateTable_t HomeRules[] = {
    { EVENT_HOME_READY, DummyFunc, &StateHome },
    { KEY_ENTER, DummyFunc, &StateDatabaseEntry },
    { KEY_ERROR, ToggleEntryError, &StateHome }
};

const StateType_t StateHome = {
    DummyFunc, NO, STATE_RULES(HomeRules), SCREEN_HOME
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
    { KEY_ENTER, OpenDatabase, &StateDatabaseContent },
    { KEY_DELETE, DummyFunc, &StateDeleteConfirm },
    { KEY_ESC, DummyFunc, &PreviousState }
};

const StateType_t StateDatabaseSelect = {
    DummyFunc, NO, STATE_RULES(DatabaseSelectRules), SCREEN_DATABASE_SELECT
};

/*========== 数据库内容：逐级返回或返回指定首页 ==========*/
static const StateTable_t DatabaseContentRules[] = {
    { KEY_UP, MoveCell, &StateDatabaseContent },
    { KEY_DOWN, MoveCell, &StateDatabaseContent },
    { KEY_LEFT, MoveCell, &StateDatabaseContent },
    { KEY_RIGHT, MoveCell, &StateDatabaseContent },
    { KEY_ENTER, BeginEdit, &StateDatabaseEdit },
    { KEY_ESC, DummyFunc, &PreviousState },
    { KEY_HOME, SelectHomeFromHistory, &FormEntryState }
};

const StateType_t StateDatabaseContent = {
    DummyFunc, NO, STATE_RULES(DatabaseContentRules), SCREEN_DATABASE_CONTENT
};

/*========== 内容编辑：范围匹配三类字符 ==========*/
static const StateTable_t DatabaseEditRules[] = {
    { KEY_ENTER, DummyFunc, &PreviousState },
    { KEY_ESC, CancelEdit, &PreviousState },
    { KEY_BACKSPACE, EraseCharacter, &StateDatabaseEdit },
    { RANGE_DIGIT, AppendCharacter, &StateDatabaseEdit },
    { RANGE_UPPER, AppendCharacter, &StateDatabaseEdit },
    { RANGE_LOWER, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL1, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL2, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL3, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL4, AppendCharacter, &StateDatabaseEdit }
};

const StateType_t StateDatabaseEdit = {
    DummyFunc, NO, STATE_RULES(DatabaseEditRules), SCREEN_DATABASE_EDIT
};

/*========== 空数据提示 ==========*/
static const StateTable_t DatabaseEmptyRules[] = {
    { KEY_ESC, DummyFunc, &StateHome }
};

const StateType_t StateDatabaseEmpty = {
    DummyFunc, NO, STATE_RULES(DatabaseEmptyRules), SCREEN_DATABASE_EMPTY
};

/*========== 入口检查错误提示 ==========*/
static const StateTable_t DatabaseErrorRules[] = {
    { KEY_ESC, DummyFunc, &StateHome }
};

const StateType_t StateDatabaseError = {
    DummyFunc, NO, STATE_RULES(DatabaseErrorRules), SCREEN_DATABASE_ERROR
};

/*========== 删除确认 ==========*/
static const StateTable_t DeleteConfirmRules[] = {
    { KEY_ESC, DummyFunc, &StateDatabaseSelect },
    { KEY_ENTER, DummyFunc, &StateDeleteWorking }
};

const StateType_t StateDeleteConfirm = {
    DummyFunc, NO, STATE_RULES(DeleteConfirmRules), SCREEN_DELETE_CONFIRM
};

/*========== 删除工作：先显示工作画面，再等待执行事件 ==========*/
static const StateTable_t DeleteWorkingRules[] = {
    { EVENT_DELETE_EXECUTE, DeleteDatabaseContent, &StateDeleteCheck }
};

const StateType_t StateDeleteWorking = {
    DummyFunc, NO, STATE_RULES(DeleteWorkingRules), SCREEN_DELETE_WORKING
};

/*========== 删除结果检查：自动进入选择页或空数据页 ==========*/
static const StateTable_t DeleteCheckRules[] = {
    { EVENT_DB_OK, DummyFunc, &StateDeleteResume },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty }
};

const StateType_t StateDeleteCheck = {
    CheckAfterDelete, YES, STATE_RULES(DeleteCheckRules), SCREEN_ID_NONE
};

/*========== 删除结果无条件过渡 ==========*/
static const StateTable_t DeleteResumeRules[] = {
    { STATE_ANY, InitDatabaseSelect, &StateDatabaseSelect }
};

static const StateType_t StateDeleteResume = {
    DummyFunc, YES, STATE_RULES(DeleteResumeRules), SCREEN_ID_NONE
};

/* 无操作占位函数：供无需业务处理的前处理或后处理使用，原样传递触发值。 */
static UI_16 DummyFunc(UI_16 trigger)
{
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

static UI_16 BeginEdit(UI_16 trigger)
{
    memcpy(g_database.editBackup, g_database.items[g_database.selected]->cells[g_database.row][g_database.column],
           sizeof(g_database.editBackup));
    return trigger;
}

static UI_16 OpenDatabase(UI_16 trigger)
{
    g_database.row = 0u;
    g_database.column = 0u;
    return trigger;
}

static UI_16 MoveCell(UI_16 trigger)
{
    if ((trigger == KEY_UP) && (g_database.row > 0u)) g_database.row--;
    if ((trigger == KEY_DOWN) && (g_database.row + 1u < DATABASE_ROWS)) g_database.row++;
    if ((trigger == KEY_LEFT) && (g_database.column > 0u)) g_database.column--;
    if ((trigger == KEY_RIGHT) && (g_database.column + 1u < DATABASE_COLUMNS)) g_database.column++;
    return trigger;
}

static UI_16 AppendCharacter(UI_16 trigger)
{
    char *contents = g_database.items[g_database.selected]->cells[g_database.row][g_database.column];
    size_t length = strlen(contents);
    if (length + 1u < DATABASE_CELL_CAPACITY)
    {
        contents[length] = (char)trigger;
        contents[length + 1u] = '\0';
    }
    return trigger;
}

static UI_16 EraseCharacter(UI_16 trigger)
{
    char *contents = g_database.items[g_database.selected]->cells[g_database.row][g_database.column];
    size_t length = strlen(contents);
    if (length > 0u)
    {
        contents[length - 1u] = '\0';
    }
    return trigger;
}

static UI_16 CancelEdit(UI_16 trigger)
{
    memcpy(g_database.items[g_database.selected]->cells[g_database.row][g_database.column], g_database.editBackup,
           sizeof(g_database.editBackup));
    return trigger;
}

static UI_16 SelectHomeFromHistory(UI_16 trigger)
{
    UI_8 position = StateGetHistoryTop();

    while (position > 0u)
    {
        if (stateHistory.states[position - 1u] == &StateHome)
        {
            stateHistory.size = position;
            break;
        }
        position--;
    }
    return trigger;
}

static UI_16 InitDatabaseSelect(UI_16 trigger)
{
    g_database.selected = 0u;
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
