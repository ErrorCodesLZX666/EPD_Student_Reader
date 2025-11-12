#include "mcu_rtc.h"
#include "log.h"
#include "stdio.h"

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

    // 构建rtc_parameter_struct结构体
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
    // 使能电源管理时钟
    rcu_periph_clock_enable(RCU_PMU);

    // 启用对备份域中寄存器的写访问
    pmu_backup_write_enable();

    // 打开振荡器 32.768 KHz
    rcu_osci_on(RCU_LXTAL);

    // 等待振荡器32.768 KHz稳定的标志设置或振荡器启动超时
    rcu_osci_stab_wait(RCU_LXTAL);

    // 配置RTC时钟源选择LXTAL=32.768 KHz晶振
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    // 开启RTC时钟
    rcu_periph_clock_enable(RCU_RTC);
    // 等到时钟同步后，再进行寄存器更新
    rtc_register_sync_wait();

    // 获取RTC时钟项选择
    // 获取备份域控制寄存器（RCU_BDCTL）
    // 的第8第9位状态
    // RTCSRC_FLAG = GET_BITS(RCU_BDCTL, 8, 9);

    // 判断是否是第一次上电
    // 如果之前没有操作过RTC_BKP0 ，则RTC_BKP0 默认为0.

    // 如果RTC_BKP0 值为 0xf234，说明已经上电过
    if (RTC_BKP0 == 0xf234)
    {
    }
    else // 备份寄存器0 如果不为 0XF234 说明是第一次上电
    {
        RTC_BKP0 = 0xf234; // 设置为0XF234，表示上电过了
        // 配置RTC时间
        // RtcTimeConfigBCD(0x23, 0x07, 0x12, 0x03, 0x12, 0x59, 0x50);
    }
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
    rtc_parameter_struct rtc_initpara_time;
    rtc_current_time_get(&rtc_initpara_time);

    printf("Current time: %d:%d:%d\n\r",
           BcdToDecimal(rtc_initpara_time.hour),
           BcdToDecimal(rtc_initpara_time.minute),
           BcdToDecimal(rtc_initpara_time.second));

    printf("%d-%d-%d\n\r",
           BcdToDecimal(rtc_initpara_time.year),
           BcdToDecimal(rtc_initpara_time.month),
           BcdToDecimal(rtc_initpara_time.date));
}

void RtcGetCurrentTimeStruct(RtcTimeType_t *datetime) {
    rtc_parameter_struct rtc_initpara_time;
    rtc_current_time_get(&rtc_initpara_time);

    datetime->year = BcdToDecimal(rtc_initpara_time.year);
    datetime->month = BcdToDecimal(rtc_initpara_time.month);
    datetime->date = BcdToDecimal(rtc_initpara_time.date);
    datetime->week = rtc_initpara_time.day_of_week;
    datetime->hour = BcdToDecimal(rtc_initpara_time.hour);
    datetime->minute = BcdToDecimal(rtc_initpara_time.minute);
    datetime->second = BcdToDecimal(rtc_initpara_time.second);
    
}
