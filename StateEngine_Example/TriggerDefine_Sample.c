#include "TriggerDefine_Sample.h"

/*
 * 范围触发器对照表：下标 = trigger - STATE_RANGE_FIRST
 * 数字、大写字母、小写字母与四段 ASCII 常用符号。
 */
const TriggerRange_t TriggerRanges[STATE_RANGE_LAST - STATE_RANGE_FIRST + 1u] = {
    { '0', '9' },
    { 'A', 'Z' },
    { 'a', 'z' },
    { ' ', '/' },
    { ':', '@' },
    { '[', '`' },
    { '{', '~' },
};
