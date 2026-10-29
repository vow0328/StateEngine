#ifndef TYPE_DEFINE_SAMPLE_H
#define TYPE_DEFINE_SAMPLE_H

#include <stdint.h>

/*=========================================================
 * 工程基础类型与通用标志（示例值，实机按项目替换）
 *
 * 本文件属于工程/平台侧，迁移引擎本身不定义这些类型。
 * 实机工程中通常由已有的公共类型头文件代替。
 *=========================================================*/

/* 基础类型：教学例程中用标准 C 类型代替实际工程类型定义 */
typedef uint8_t UI_8;
typedef uint16_t UI_16;
typedef int BOOL;

/* 通用返回值 / 标志 */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef OK
#define OK 1
#endif
#ifndef NG
#define NG 0
#endif
#ifndef YES
#define YES 1u
#endif
#ifndef NO
#define NO 0u
#endif
#ifndef PUBLIC
#define PUBLIC
#endif

/* 无有效画面的占位屏幕 ID */
#ifndef SCREEN_ID_NONE
#define SCREEN_ID_NONE 0u
#endif

#endif /* TYPE_DEFINE_SAMPLE_H */
