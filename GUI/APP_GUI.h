#ifndef __APP_GUI_H__
#define __APP_GUI_H__

#include "gd32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "EPD.h"
#include "ff.h"
#include <stdint.h>



/* 颜色占位：请替换为你工程中实际颜色常量（比如 WHITE / BLACK / 0 / 1 等） */
#ifndef COLOR_WHITE
#define COLOR_WHITE WHITE
#endif
#ifndef COLOR_BLACK
#define COLOR_BLACK BLACK
#endif

/* 字体大小：仅允许 12 / 16 / 24 */
#define FONT_SIZE_12 12
#define FONT_SIZE_16 16
#define FONT_SIZE_24 24

/* 画布尺寸（由 EPD.h 中的 EPD_W / EPD_H 提供）*/
#define SCREEN_W EPD_W
#define SCREEN_H EPD_H

/* 基础类型别名（统一在头文件定义） */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;


typedef struct {
    const char** novel_list;  // 小说文件字符串数组
    uint16_t novel_count;     // 小说总个数
    uint16_t current_index;   // 当前选中索引
    const char *time_str;     // 顶部时间字符串，例如 "12:35"
} NovelListBox_t;


/* 接口 */
static uint16_t DrawCenteredString(u16 y, const char *s, u16 size, u16 color);
void UI_DrawLockScreen(int hour, int minute, int year, int mouth, int day);
void UI_UpdateLockDatetime(int hour, int minute, int year, int mouth, int day);
void UI_DrawLoadingScreen(const char *title, const char *subtitle);
void UI_DrawReaderPage(const char *heading, const char *progress_text, const char *content, int progress);
uint32_t UI_CalcReaderPageChars(const char *content, uint8_t font_size);                // 定义一个方法用来计算一页能显示多少字符
uint32_t UI_CalcReaderPageBytes(const char *content, uint8_t font_size);                // 定义一个方法用来计算一页能显示多少字节
void UI_DrawNovelListBox(const NovelListBox_t *box);
// 更新错误显示界面
void UI_DrawErrorScreen(const char *error_msg);
// 建立索引界面
void UI_DrawIndexLoadingScreen(uint32_t currentProgress, uint32_t totalProgress, uint8_t finish_flags);


/* 尝试定义更新时间的接口 */
#include "mcu_rtc.h"            // 统一使用RTC定义的时间结构体
void UI_flushTime_LockScreen(const RtcTimeType_t *time);
// void UI_flushTime_NovelListBox(const RtcTimeType_t *time);
// void UI_flushTime_ReaderPage(const RtcTimeType_t *time);


void UI_Show(void);
// 如果想要使用局部刷新就必须得先初始化才能使用局部刷新哦
void UI_PartShowInit(void);
void UI_PartShow(void);

/* 可选：如果你的字体库提供测量函数，声明它以便 DrawCenteredString 使用
   如果没有，你可以实现下面的回退函数：EPD_GetStringWidth_Fallback */
u16 EPD_GetStringWidth(const char *s, u16 size); /* 若存在请在字体库中实现并导出 */

#endif 
