#ifndef __MCU_RTC_H__
#define __MCU_RTC_H__

#include "gd32f4xx.h"

// 定义一个给用户使用的RTC时间结构体
typedef struct
{
    uint8_t year;        // 年份
    uint8_t month;       // 月份
    uint8_t date;        // 日期
    uint8_t week;        // 星期
    uint8_t hour;        // 小时
    uint8_t minute;      // 分钟
    uint8_t second;      // 秒
} RtcTimeType_t;

void rtc_config(void);
void RtcShowTime(void);
void RtcTimeConfigBCD(uint8_t year, uint8_t month, uint8_t date, uint8_t week, uint8_t hour, uint8_t minute, uint8_t second);
void RtcTimeConfigStruct(RtcTimeType_t *datetime);
void RtcGetCurrentTimeStruct(RtcTimeType_t *datetime);

// 定义每分钟循环触发一次闹钟功能
void RtcEachMinuteLoopEnable(void);
// extern void RtcAlarmPerMinute_cb(void);

int BcdToDecimal(int bcd);
int DecimalToBcd(int decimal);
#endif