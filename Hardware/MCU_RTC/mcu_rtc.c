#include "mcu_rtc.h"
#include "log.h"
#include "stdio.h"
#include "fat_app.h"
#include "FreeRTOS.h"
#include "task.h"

/**********************************************************
 * 函 数 名 称：DecimalToBcd
 * 函 数 功 能：10进制转BCD
 * 传 入 参 数：decimal = 10进制数
 * 函 数 返 回：转换完成的BCD码
 * 作       者：LCKFB
 * 备       注：无
 **********************************************************/
int DecimalToBcd(int decimal)
{
    int bcd = 0;
    int temp = 1;
    int number = 0;

    while (decimal > 0)
    {
        number = decimal % 10;
        bcd += number * temp;
        temp *= 16;
        decimal /= 10;
    }
    return bcd;
}
/**********************************************************
 * 函 数 名 称：BcdToDecimal
 * 函 数 功 能：BCD转10进制
 * 传 入 参 数：bcd = BCD码
 * 函 数 返 回：转换完成的10进制
 * 作       者：LCKFB
 * 备       注：无
 **********************************************************/
int BcdToDecimal(int bcd)
{
    int decimal = 0;
    int temp = 1;
    int number = 0;

    while (bcd > 0)
    {
        number = bcd % 16;
        decimal += number * temp;
        temp *= 10;
        bcd /= 16;
    }
    return decimal;
}

// 十位取出左移4位 + 个位 (得到BCD数)
#define WRITE_BCD(val) ((val / 10) << 4) + (val % 10)
// 将高4位乘以10 + 低四位 (得到10进制数)
#define READ_BCD(val) (val >> 4) * 10 + (val & 0x0F)

void RtcTimeConfigStruct(RtcTimeType_t *datetime)
{
    // 计算结构体中道 BCD
    uint16_t year = DecimalToBcd(datetime->year);
    uint8_t month = DecimalToBcd(datetime->month);
    uint8_t date = DecimalToBcd(datetime->date);
    uint8_t week = datetime->week;
    uint8_t hour = DecimalToBcd(datetime->hour);
    uint8_t minute = DecimalToBcd(datetime->minute);
    uint8_t second = DecimalToBcd(datetime->second);

    LOGW("RTC Time will Config to: 20%02d-%02d-%02d %02d:%02d:%02d Weekday:%d\r\n",
         datetime->year, datetime->month, datetime->date,
         datetime->hour, datetime->minute, datetime->second,
         datetime->week);
    LOGW("RTC Time will Config to (BCD): 20%02d-%02d-%02d %02d:%02d:%02d Weekday:%d\r\n",
         year, month, date,
         hour, minute, second,
         week);

    // 构建rtc_parameter_struct结构体
    rtc_parameter_struct rtc_initpara;
    rtc_initpara.factor_asyn = 0x7F; // RTC异步预分频值:0x0 ~ 0x7F
    rtc_initpara.factor_syn = 0xFF;  // RTC同步预分频值:0x0 - 0x7FFF
    rtc_initpara.year = year;        // 设置年份
    rtc_initpara.month = month;      // 设置月份
    rtc_initpara.date = date;        // 设置日期
    // rtc_initpara.day_of_week = week;          // 设置星期
    rtc_initpara.hour = hour;                 // 设置时
    rtc_initpara.minute = minute;             // 设置分钟
    rtc_initpara.second = second;             // 设置秒
    rtc_initpara.display_format = RTC_24HOUR; // 24小时制

    // RTC当前时间配置
    rtc_init(&rtc_initpara);
}

void RtcTimeConfigBCD(uint8_t year, uint8_t month, uint8_t date, uint8_t week, uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_parameter_struct rtc_initpara;
    rtc_initpara.factor_asyn = 0x7F;          // RTC异步预分频值:0x0 ~ 0x7F
    rtc_initpara.factor_syn = 0xFF;           // RTC同步预分频值:0x0 - 0x7FFF
    rtc_initpara.year = year;                 // 设置年份
    rtc_initpara.month = month;               // 设置月份
    rtc_initpara.date = date;                 // 设置日期
    rtc_initpara.day_of_week = week;          // 设置星期
    rtc_initpara.hour = hour;                 // 设置时
    rtc_initpara.minute = minute;             // 设置分钟
    rtc_initpara.second = second;             // 设置秒
    rtc_initpara.display_format = RTC_24HOUR; // 24小时制
                                              //     rtc_initpara.am_pm = RTC_PM;//午后  //12小时制才使用到
    // RTC当前时间配置
    rtc_init(&rtc_initpara);
}

void rtc_config(void)
{
    // 电池管理加载
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    // 重置备份域（不重置可能导致无法设置晶振，出现不走字情况）
    /* reset backup domain */
    rcu_bkp_reset_enable();
    rcu_bkp_reset_disable();

    // 外部晶振加载
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    // 内部晶振
    // rcu_osci_on(RCU_IRC32K);
    // rcu_osci_stab_wait(RCU_IRC32K);
    // rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

    // RTC功能加载
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    rtc_parameter_struct rps;
    rps.year = WRITE_BCD(23);
    rps.month = WRITE_BCD(4);
    rps.date = WRITE_BCD(20);
    rps.day_of_week = WRITE_BCD(4);
    rps.hour = WRITE_BCD(23);
    rps.minute = WRITE_BCD(59);
    rps.second = WRITE_BCD(55);
    rps.display_format = RTC_24HOUR;
    rps.am_pm = RTC_AM;
    rps.factor_asyn = 0x7F;
    rps.factor_syn = 0xFF;

    rtc_init(&rps);
}

/**********************************************************
 * 函 数 名 称：RtcShowTime
 * 函 数 功 能：获取RTC时间并显示
 * 传 入 参 数：无
 * 函 数 返 回：无
 * 作       者：LCKFB
 * 备       注：该函数是通过串口输出时间的方式显示，可以更改为其他方式
 **********************************************************/

void RtcShowTime(void)
{
    rtc_parameter_struct rps;
    rtc_current_time_get(&rps);

    uint16_t year = READ_BCD(rps.year) + 2000;
    uint8_t month = READ_BCD(rps.month);
    uint8_t date = READ_BCD(rps.date);
    uint8_t week = READ_BCD(rps.day_of_week);
    uint8_t hour = READ_BCD(rps.hour);
    uint8_t minute = READ_BCD(rps.minute);
    uint8_t second = READ_BCD(rps.second);

    printf("%d-%d-%d %d %d:%d:%d\r\n", year, month, date, week, hour, minute, second);
}

void RtcGetCurrentTimeStruct(RtcTimeType_t *datetime)
{
    rtc_parameter_struct rps;
    rtc_current_time_get(&rps);

    datetime->year = READ_BCD(rps.year);
    datetime->month = READ_BCD(rps.month);
    datetime->date = READ_BCD(rps.date);
    datetime->week = READ_BCD(rps.day_of_week);
    datetime->hour = READ_BCD(rps.hour);
    datetime->minute = READ_BCD(rps.minute);
    datetime->second = READ_BCD(rps.second);
}

// 定义一分钟触发一次的RTC闹钟功能
void RTC_Alarm_IRQHandler(void) // 回调中断函数
{
    LOGI("RTC per minute Alarm Interrupt Triggered!\r\n");
    if (rtc_flag_get(RTC_FLAG_ALRM0) != RESET)
    {
        rtc_flag_clear(RTC_FLAG_ALRM0);

        /* 中断中的提示逻辑 */
        LOGI("RTC per minute Alarm0 Interrupt Triggered!\r\n");
        // RtcAlarmPerMinute_cb(); // 调用用户定义的每分钟闹钟处理回调函数
        /* 重新设置下一分钟的 Alarm */
        RtcEachMinuteLoopEnable();
    }
}

void RtcEachMinuteLoopEnable(void)
{
    rtc_alarm_struct alarm;
    rtc_parameter_struct current;

    rtc_current_time_get(&current);

    /* 屏蔽 日期、小时，只匹配 分钟和秒 */
    alarm.alarm_mask = RTC_ALARM_DATE_MASK | RTC_ALARM_HOUR_MASK;

    alarm.weekday_or_date = 0;
    alarm.am_pm = 0;

    alarm.alarm_day = 0;  // 屏蔽，不需要
    alarm.alarm_hour = 0; // 屏蔽，不需要

    alarm.alarm_minute = (current.minute + 1) % 60;
    alarm.alarm_second = 0; // 必须匹配 00 秒

    /* 配置闹钟 */
    rtc_alarm_disable(RTC_ALARM0);
    rtc_flag_clear(RTC_FLAG_ALRM0);

    rtc_alarm_config(RTC_ALARM0, &alarm);
    rtc_alarm_enable(RTC_ALARM0);

    rtc_interrupt_enable(RTC_INT_ALARM0);
    nvic_irq_enable(RTC_Alarm_IRQn, 2, 0);
}
