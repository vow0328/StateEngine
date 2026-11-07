#include "Action_StateTransition_Sample.h"
#include "Screen_StateTransition_Sample.h"
#include <string.h>

static DatabaseItem g_databaseItems[] =
{
    { "Products",  {{ "P001", "Keyboard" }, { "P002", "Mouse" }} },
    { "Orders",    {{ "O001", "Pending" }, { "O002", "Completed" }} },
    { "Customers", {{ "C001", "Alice" }, { "C002", "Bob" }} }
};

static DatabaseContext g_database =
{
    { &g_databaseItems[0], &g_databaseItems[1], &g_databaseItems[2] },3u,0u,0u,0u,NO,
    { 0 }
};

static UI_16 DummyFunc(UI_16 trigger);
static UI_16 CheckDatabaseSelect(UI_16 trigger);
static UI_16 CheckDatabaseItem(UI_16 trigger);
static UI_16 CheckCell(UI_16 trigger);
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
static const StateType_t StateHomeEntry;
static const StateType_t StateContentEntry;
static const StateType_t StateEditEntry;
static const StateType_t StateConfirmEntry;
static const StateType_t StateWorkingEntry;
static const StateType_t StateDeleteResume;
static const StateType_t StateBackEntry;
static const StateType_t StateHomeBackEntry;

/*
 * StateType_t 定义状态表；StateTable_t 定义迁移表，每个数组元素是一个迁移表项。
 * 每组先列迁移表数组，再列通过 transTbl 引用该数组的状态表。
 * 跨画面迁移先经过 autoFlg=YES 的入口或回退状态表；页内操作直接自迁移。
 */

/*========== 首页入口：自动过渡到首页，不重置模拟数据 ==========*/
static const StateTable_t HomeEntryRules[] =
{
    { STATE_ANY, DummyFunc, &StateHome }
};

static const StateType_t StateHomeEntry =
{
    DummyFunc,
    YES,
    ListOf(HomeEntryRules),
    SCREEN_ID_NONE
};

/*========== 首页状态表及其迁移表 ==========*/
static const StateTable_t HomeRules[] =
{
    { EVENT_HOME_READY, DummyFunc,        &StateHomeEntry },
    { KEY_ENTER,        DummyFunc,        &StateDatabaseEntry },
    { KEY_ERROR,        ToggleEntryError, &StateHome }
};

const StateType_t StateHome =
{
    DummyFunc,
    NO,
    ListOf(HomeRules),
    SCREEN_HOME
};

/*========== 选择页入口状态表及其迁移表：跨页面进入时自动检查 ==========*/
static const StateTable_t DatabaseEntryRules[] =
{
    { EVENT_DB_ERROR, DummyFunc,          &StateDatabaseError },
    { EVENT_DB_EMPTY, DummyFunc,          &StateDatabaseEmpty },
    { EVENT_DB_OK,    InitDatabaseSelect, &StateDatabaseSelect }
};

const StateType_t StateDatabaseEntry =
{
    CheckDatabaseSelect,
    YES,
    ListOf(DatabaseEntryRules),
    SCREEN_ID_NONE
};

/*========== 数据库选择状态表及其迁移表：页内操作不经过入口检查 ==========*/
static const StateTable_t DatabaseSelectRules[] =
{
    { KEY_UP,     MoveDatabasePrevious, &StateDatabaseSelect },
    { KEY_DOWN,   MoveDatabaseNext,     &StateDatabaseSelect },
    { KEY_ENTER,  DummyFunc,            &StateContentEntry },
    { KEY_DELETE, DummyFunc,            &StateConfirmEntry },
    { KEY_ESC,    DummyFunc,            &StateBackEntry }
};

const StateType_t StateDatabaseSelect =
{
    DummyFunc,
    NO,
    ListOf(DatabaseSelectRules),
    SCREEN_DATABASE_SELECT
};

/*========== 内容页入口：检查数据库，再初始化内容页光标 ==========*/
static const StateTable_t ContentEntryRules[] =
{
    { EVENT_DB_ERROR, DummyFunc,    &StateDatabaseError },
    { EVENT_DB_EMPTY, DummyFunc,    &StateDatabaseEmpty },
    { EVENT_DB_OK,    OpenDatabase, &StateDatabaseContent }
};

static const StateType_t StateContentEntry =
{
    CheckDatabaseItem,
    YES,
    ListOf(ContentEntryRules),
    SCREEN_ID_NONE
};

/*========== 数据库内容状态表及其迁移表：逐级返回或返回指定首页 ==========*/
static const StateTable_t DatabaseContentRules[] =
{
    { KEY_UP,    MoveCell,  &StateDatabaseContent },
    { KEY_DOWN,  MoveCell,  &StateDatabaseContent },
    { KEY_LEFT,  MoveCell,  &StateDatabaseContent },
    { KEY_RIGHT, MoveCell,  &StateDatabaseContent },
    { KEY_ENTER, DummyFunc, &StateEditEntry },
    { KEY_ESC,   DummyFunc, &StateBackEntry },
    { KEY_HOME,  DummyFunc, &StateHomeBackEntry }
};

const StateType_t StateDatabaseContent =
{
    DummyFunc,
    NO,
    ListOf(DatabaseContentRules),
    SCREEN_DATABASE_CONTENT
};

/*========== 编辑页入口：检查单元格，备份本次修改前的内容 ==========*/
static const StateTable_t EditEntryRules[] =
{
    { EVENT_DB_ERROR, DummyFunc, &StateDatabaseError },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty },
    { EVENT_DB_OK,    BeginEdit, &StateDatabaseEdit }
};

static const StateType_t StateEditEntry =
{
    CheckCell,
    YES,
    ListOf(EditEntryRules),
    SCREEN_ID_NONE
};

/*========== 内容编辑状态表及其迁移表：按范围条件匹配三类字符 ==========*/
static const StateTable_t DatabaseEditRules[] =
{
    { KEY_ENTER,     DummyFunc,       &StateBackEntry },
    { KEY_ESC,       CancelEdit,      &StateBackEntry },
    { KEY_BACKSPACE, EraseCharacter,  &StateDatabaseEdit },
    { RANGE_DIGIT,   AppendCharacter, &StateDatabaseEdit },
    { RANGE_UPPER,   AppendCharacter, &StateDatabaseEdit },
    { RANGE_LOWER,   AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL1, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL2, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL3, AppendCharacter, &StateDatabaseEdit },
    { RANGE_SYMBOL4, AppendCharacter, &StateDatabaseEdit }
};

const StateType_t StateDatabaseEdit =
{
    DummyFunc,
    NO,
    ListOf(DatabaseEditRules),
    SCREEN_DATABASE_EDIT
};

/*========== 空数据提示状态表及其迁移表 ==========*/
static const StateTable_t DatabaseEmptyRules[] =
{
    { KEY_ESC, DummyFunc, &StateHomeEntry }
};

const StateType_t StateDatabaseEmpty =
{
    DummyFunc,
    NO,
    ListOf(DatabaseEmptyRules),
    SCREEN_DATABASE_EMPTY
};

/*========== 入口错误提示状态表及其迁移表 ==========*/
static const StateTable_t DatabaseErrorRules[] =
{
    { KEY_ESC, DummyFunc, &StateHomeEntry }
};

const StateType_t StateDatabaseError =
{
    DummyFunc,
    NO,
    ListOf(DatabaseErrorRules),
    SCREEN_DATABASE_ERROR
};

/*========== 删除确认入口：检查将要显示的数据库是否有效 ==========*/
static const StateTable_t ConfirmEntryRules[] =
{
    { EVENT_DB_ERROR, DummyFunc, &StateDatabaseError },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty },
    { EVENT_DB_OK,    DummyFunc, &StateDeleteConfirm }
};

static const StateType_t StateConfirmEntry =
{
    CheckDatabaseItem,
    YES,
    ListOf(ConfirmEntryRules),
    SCREEN_ID_NONE
};

/*========== 删除确认状态表及其迁移表 ==========*/
static const StateTable_t DeleteConfirmRules[] =
{
    { KEY_ESC,   DummyFunc, &StateBackEntry },
    { KEY_ENTER, DummyFunc, &StateWorkingEntry }
};

const StateType_t StateDeleteConfirm =
{
    DummyFunc,
    NO,
    ListOf(DeleteConfirmRules),
    SCREEN_DELETE_CONFIRM
};

/*========== 删除工作入口：再次检查目标，再进入可显示的工作状态 ==========*/
static const StateTable_t WorkingEntryRules[] =
{
    { EVENT_DB_ERROR, DummyFunc, &StateDatabaseError },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty },
    { EVENT_DB_OK,    DummyFunc, &StateDeleteWorking }
};

static const StateType_t StateWorkingEntry =
{
    CheckDatabaseItem,
    YES,
    ListOf(WorkingEntryRules),
    SCREEN_ID_NONE
};

/*========== 删除工作状态表及其迁移表：先显示工作画面，再等待执行事件 ==========*/
static const StateTable_t DeleteWorkingRules[] =
{
    { EVENT_DELETE_EXECUTE, DeleteDatabaseContent, &StateDeleteCheck }
};

const StateType_t StateDeleteWorking =
{
    DummyFunc,
    NO,
    ListOf(DeleteWorkingRules),
    SCREEN_DELETE_WORKING
};

/*========== 删除结果检查状态表及其迁移表：自动进入选择页或空数据页 ==========*/
static const StateTable_t DeleteCheckRules[] =
{
    { EVENT_DB_OK,    DummyFunc, &StateDeleteResume },
    { EVENT_DB_EMPTY, DummyFunc, &StateDatabaseEmpty }
};

const StateType_t StateDeleteCheck =
{
    CheckAfterDelete,
    YES,
    ListOf(DeleteCheckRules),
    SCREEN_ID_NONE
};

/*========== 删除结果过渡状态表及其迁移表：STATE_ANY 无条件匹配 ==========*/
static const StateTable_t DeleteResumeRules[] =
{
    { STATE_ANY, InitDatabaseSelect, &StateDatabaseSelect }
};

static const StateType_t StateDeleteResume =
{
    DummyFunc,
    YES,
    ListOf(DeleteResumeRules),
    SCREEN_ID_NONE
};

/*========== 逐级回退入口：自动解析历史，不重复初始化目标画面 ==========*/
static const StateTable_t BackEntryRules[] =
{
    { STATE_ANY, DummyFunc, &PreviousState }
};

static const StateType_t StateBackEntry =
{
    DummyFunc,
    YES,
    ListOf(BackEntryRules),
    SCREEN_ID_NONE
};

/*========== 指定首页回退入口：截断历史后自动解析 FormEntryState ==========*/
static const StateTable_t HomeBackEntryRules[] =
{
    { STATE_ANY, SelectHomeFromHistory, &FormEntryState }
};

static const StateType_t StateHomeBackEntry =
{
    DummyFunc,
    YES,
    ListOf(HomeBackEntryRules),
    SCREEN_ID_NONE
};

/* 无操作占位函数：供状态表的前处理或迁移表项的后处理使用，原样传递触发值。 */
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

static UI_16 CheckDatabaseItem(UI_16 trigger)
{
    (void)trigger;
    if (g_database.count == 0u)
    {
        return EVENT_DB_EMPTY;
    }
    if ((g_database.selected >= g_database.count) ||
        (g_database.items[g_database.selected] == NULL))
    {
        return EVENT_DB_ERROR;
    }
    return EVENT_DB_OK;
}

static UI_16 CheckCell(UI_16 trigger)
{
    UI_16 result = CheckDatabaseItem(trigger);

    if (result != EVENT_DB_OK)
    {
        return result;
    }
    if ((g_database.row >= DATABASE_ROWS) || (g_database.column >= DATABASE_COLUMNS))
    {
        return EVENT_DB_ERROR;
    }
    return EVENT_DB_OK;
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
            stateHistory.top = position;
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
