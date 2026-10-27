#include "TriggerDefine_Sample.h"

/*
 * 范围触发器对照表：下标 = trig - MC_TOP
 * 索引 0：0x0100～0x0102（数字事件）
 * 索引 1：0x0200～0x0202（字母事件）
 * 其余下标未显式赋值，由编译器零初始化为 {0, 0}，不会命中。
 */
const MinToMaxTrig_T TblMcTrig[MC_END - MC_TOP + 1u] = {
    { (UI_16)0x0100u, (UI_16)0x0102u }, /* 数字事件 */
    { (UI_16)0x0200u, (UI_16)0x0202u }, /* 字母事件 */
};
