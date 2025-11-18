#include "APP_GUI.h"
#include "EPD_GUI.h"
#include "EPD.h"
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "img_reader_icon.h"

// 定义一个局部刷新次数，让他达到了这个次数就会刷新
uint8_t partUpdateCounter = 0; // 当墨水屏>= 50 的时候就会显示错误。
/* 画布缓冲：1bpp 假设（请确认 EPD_NewImage 要求） */
static u8 g_image_buf[(SCREEN_W * SCREEN_H) / 8];

/* 回退的字符串宽度估算（当你的字体库没有提供测量函数时使用）
   估算规则：ASCII 字符宽度约 = size/2；中文（UTF-8 多字节）宽度约 = size（正方） */
static u16 EPD_GetStringWidth_Fallback(const char *s, u16 size)
{
    if (!s)
        return 0;
    u16 width = 0;
    const unsigned char *p = (const unsigned char *)s;
    while (*p)
    {
        if (*p < 0x80)
        {
            width += size / 2;
            p++;
        }
        else if (*p >= 0x81 && *p <= 0xFE && *(p + 1) >= 0x40)
        {
            width += size; // 中文宽度 = 字号大小
            p += 2;
        }
        else
        {
            p++; // 跳过异常字节
        }
    }
    return width;
}

/* 尝试调用外部测量函数（若存在），否则使用回退估算 */
static u16 GetStringWidth(const char *s, u16 size)
{
    /* 如果你在字库中实现了 EPD_GetStringWidth，请让链接使用它；
       下面做弱引用调用：如果存在就用，否则用回退 */
#ifdef USE_FONT_MEASURE_FUNC
    return EPD_GetStringWidth(s, size);
#else
    (void)EPD_GetStringWidth; /* 至少引用避免编译器警告（若未实现） */
    return EPD_GetStringWidth_Fallback(s, size);
#endif
}

/* 在水平中心绘制字符串（单行） */
static uint16_t DrawCenteredString(u16 y, const char *s, u16 size, u16 color)
{
    if (!s)
        return 0;

    /* 检查允许的字号 */
    if (size != FONT_SIZE_12 && size != FONT_SIZE_16 && size != FONT_SIZE_24)
    {
        LOGE("Invalid font size %d, reset to 16\n", size);
        size = FONT_SIZE_16;
    }

    u16 text_w = GetStringWidth(s, size);
    u16 x = 0;
    if (text_w < SCREEN_H)
        x = (SCREEN_H - text_w) / 2;
    else
        x = 0;

    /* 调用显示函数（你的 EPD_ShowString 或 EPD_ShowChinese 根据编码选择） */
    EPD_ShowChinese(x, y, (u8 *)s, size, color);
    return text_w;
}

static uint16_t DrawCenteredTime(u16 y, const char *s, u16 size, u16 color)
{
    if (!s)
        return 0;

    /* 检查允许的字号 */
    if (size != FONT_SIZE_12 && size != FONT_SIZE_16 && size != FONT_SIZE_24 && size != 48)
    {
        LOGE("Invalid font size %d, reset to 16\n", size);
        size = FONT_SIZE_16;
    }

    u16 text_w = GetStringWidth(s, size);
    u16 x = 0;
    if (text_w < SCREEN_H)
        x = (SCREEN_H - text_w) / 2;
    else
        x = 0;

    /* 调用显示函数（你的 EPD_ShowString 或 EPD_ShowChinese 根据编码选择） */
    EPD_ShowString(x, y, (u8 *)s, size, color);
    return text_w;
}

// 功能：通过年月日计算星期几
// 返回值：0=星期日, 1=星期一, ..., 6=星期六
uint8_t Get_Weekday(uint16_t year, uint8_t month, uint8_t day)
{
    // 使用蔡勒（Zeller）公式
    if (month < 3)
    {
        month += 12;
        year -= 1;
    }

    uint16_t K = year % 100; // 年份的后两位
    uint16_t J = year / 100; // 世纪数

    uint8_t h = (day + (13 * (month + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;

    // 蔡勒公式结果中：0=星期六, 1=星期日, 2=星期一, ...
    // 转换为 0=星期日, 1=星期一, ..., 6=星期六
    uint8_t week = (h + 6) % 7;
    return week;
}

char weekend_str[7][10] = {
    "天\0",
    "一\0",
    "二\0",
    "三\0",
    "四\0",
    "五\0",
    "六\0",
};
/* --------------------- 1) 锁屏钟面 --------------------- */
/* hour/minute 改为 int 以便避免浮点依赖 */
void UI_DrawLockScreen(int hour, int minute, int year, int mouth, int day)
{
    Paint_NewImage(g_image_buf, SCREEN_W, SCREEN_H, 0, COLOR_WHITE);
    Paint_Clear(COLOR_WHITE);
    /* 边框 */
    EPD_DrawRectangle(2, 2, SCREEN_H - 3, SCREEN_W - 3, COLOR_BLACK, 0);

    /* 时间（HH:MM） — 居中显示，采用大字号 24 */
    char timestr[16], date_str[20];
    snprintf(timestr, sizeof(timestr), "%02d:%02d", hour, minute);
    snprintf(date_str, sizeof(date_str), "%04d年%02d月%02d日 周%s", year, mouth, day, weekend_str[Get_Weekday(year, mouth, day)]);
    // DrawCenteredTime(70, timestr, 48, COLOR_BLACK);
    EPD_ShowString(124, 33, (u8 *)timestr, 48, COLOR_BLACK);
    /* 日期行 */
    if (date_str)
    {
        EPD_ShowChinese(84, 91, date_str, FONT_SIZE_16, COLOR_BLACK);
        // DrawCenteredString(130, date_str, FONT_SIZE_16, COLOR_BLACK);
    }

    /* 下方提示行 */
    // DrawCenteredString(SCREEN_W - 40, "按下任意键启用屏幕", FONT_SIZE_12, COLOR_BLACK);

    // 绘制中间条 3 px 像素
    for (uint8_t counter = 0; counter < 3; counter++)
    {
        EPD_DrawLine(257 + counter, 34, 257 + counter, SCREEN_W - 34, COLOR_BLACK);
    }

    // EPD_ShowPicture(10,10,16,16,gImage_temperature_icon,COLOR_WHITE);
    //  绘制两个Icon
    //  温度图标
    EPD_ShowPicture(276, 133, 16, 16, gImage_temperature_icon, COLOR_WHITE);
    // 湿度图标
    EPD_ShowPicture(276, 168, 16, 16, gImage_humidity_icon, COLOR_WHITE);
    // 显示温度和湿度
    EPD_ShowChinese(297, 133, (u8 *)"N/A", FONT_SIZE_16, COLOR_BLACK);
    EPD_ShowChinese(297, 168, (u8 *)"N/A", FONT_SIZE_16, COLOR_BLACK);
}

// 刷新锁屏的实现（使用墨水屏的局部刷新）
void UI_UpdateLockDatetime(int hour, int minute, int year, int mouth, int day)
{
    char timestr[16], date_str[20];
    snprintf(timestr, sizeof(timestr), "%02d:%02d", hour, minute);
    snprintf(date_str, sizeof(date_str), "%04d年%02d月%02d日 周%s", year, mouth, day, weekend_str[Get_Weekday(year, mouth, day)]);
    DrawCenteredString(100, timestr, FONT_SIZE_24, COLOR_BLACK);

    /* 日期行 */
    if (date_str)
    {
        DrawCenteredString(130, date_str, FONT_SIZE_16, COLOR_BLACK);
    }
}

/* --------------------- 2) Loading 界面 --------------------- */
void UI_DrawLoadingScreen(const char *title, const char *subtitle)
{
    Paint_NewImage(g_image_buf, SCREEN_W, SCREEN_H, 0, COLOR_WHITE);
    Paint_Clear(COLOR_WHITE);
    EPD_DrawRectangle(2, 2, SCREEN_H - 3, SCREEN_W - 3, COLOR_BLACK, 0);

    /* 图标（居中）*/
    u16 icon_w = 80;
    u16 icon_x = (SCREEN_H - icon_w) / 2;
    u16 icon_y = 60;
    // TODOS ： 这里使用的是旧版本的绘制程序
    // EPD_DrawRectangle(icon_x, icon_y, icon_x + icon_w, icon_y + icon_w, COLOR_BLACK, 0);
    // EPD_DrawLine(icon_x + 18, icon_y + 10, icon_x + 18, icon_y + 70, COLOR_BLACK);
    // EPD_DrawLine(icon_x + 28, icon_y + 30, icon_x + 72, icon_y + 30, COLOR_BLACK);
    // EPD_DrawLine(icon_x + 28, icon_y + 45, icon_x + 72, icon_y + 45, COLOR_BLACK);
    // 尝试使用图片作为Icon
    EPD_ShowPicture(icon_x, icon_y, icon_w, icon_w, gImage_img_reader_icon, COLOR_WHITE);

    if (title)
        DrawCenteredString(icon_y + icon_w + 24, title, FONT_SIZE_16, COLOR_BLACK);
    if (subtitle)
        DrawCenteredString(icon_y + icon_w + 48, subtitle, FONT_SIZE_12, COLOR_BLACK);

    // 打印启动提示
    DrawCenteredString(SCREEN_H - 30, "系统正在尝试启动中", FONT_SIZE_12, COLOR_BLACK);
    // 打印固件版本号
    EPD_ShowString(SCREEN_H - 50, SCREEN_W - 20, (u8 *)"VER1.0", FONT_SIZE_12, COLOR_BLACK);
}

/* --------------------- 3) 阅读器主页面 --------------------- */
void UI_DrawReaderPage(const char *heading, const char *progress_text, const char *content, int progress)
{
    Paint_NewImage(g_image_buf, SCREEN_W, SCREEN_H, 0, COLOR_WHITE);
    Paint_Clear(COLOR_WHITE);

    /* 顶部标题区 */
    EPD_DrawRectangle(4, 4, SCREEN_H - 5, 36, COLOR_BLACK, 0);
    if (heading)
        EPD_ShowChinese(8, 10, (u8 *)heading, FONT_SIZE_12, COLOR_BLACK);

    if (progress_text)
    {
        u16 text_w = GetStringWidth(progress_text, FONT_SIZE_12);
        u16 x = (text_w < SCREEN_H) ? (SCREEN_H - 8 - text_w) : (SCREEN_H - 8);
        EPD_ShowChinese(x, 10, (u8 *)progress_text, FONT_SIZE_12, COLOR_BLACK);
    }

    /* 进度条 */
    u16 bar_x = 8, bar_y = 26, bar_w = SCREEN_H - 16, bar_h = 6;
    EPD_DrawRectangle(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, COLOR_BLACK, 0);
    u16 fill_w = (u16)(((int)(bar_w - 2) * progress) / 100);
    if (fill_w > 0)
    {
        EPD_DrawRectangle(bar_x + 1, bar_y + 1, bar_x + 1 + fill_w, bar_y + bar_h, COLOR_BLACK, 1);
    }

    /* 正文区域（左侧文本，右侧滚动条） */
    u16 text_x = 8;
    u16 text_y = 44;
    // u16 text_h = SCREEN_W - text_y - 16;

    /* 换行：使用基于像素宽度的换行（利用 GetStringWidth 估算） */
    const char *p = content;
    u16 cur_y = text_y;
    const u16 line_height = FONT_SIZE_16 + 4; /* 行高可调整 */
    char linebuf[256];

    while (*p && (cur_y + line_height <= SCREEN_W - 10))
    {
        /* 逐字符累加直到超出文本区域宽度 */
        const char *start = p;
        int used = 0;

        /* 检查换行符 */
        if (*p == '\r' || *p == '\n')
        {
            // 处理 \r\n 情况
            if (*p == '\r' && *(p + 1) == '\n')
                p += 2;
            else
                p += 1;
            cur_y += line_height; // Y 向下移一行
            continue;             // 开始下一行
        }

        u16 w = 0;
        while (*p)
        {
            /* 为避免复杂的逐字度量，这里把每次扩展为一个字节（UTF-8 可能截断多字节字符）
               最稳妥是以字为单位（需你的字体库支持），如果有请替换下面逻辑。 */
            // 手动换行符再检测一次
            if (*p == '\r' || *p == '\n')
                break;
            uint8_t c = (uint8_t)*p;
            int char_len = (c >= 0x81 && c <= 0xFE) ? 2 : 1;
            if (char_len == 2)
            {
                linebuf[used] = *p;
                linebuf[used + 1] = *(p + 1);
                linebuf[used + 2] = 0;
            }
            else
            {
                linebuf[used] = *p;
                linebuf[used + 1] = 0;
            }

            u16 tw = GetStringWidth(linebuf, FONT_SIZE_16);
            if (tw > (SCREEN_H - 15))
            {
                if (char_len == 2)
                {
                    linebuf[used] = 0;
                    linebuf[used + 1] = 0;
                }
                else
                {
                    linebuf[used] = 0;
                }
                break;
            }

            used += char_len;
            p += char_len;
            if (used >= (int)(sizeof(linebuf) - 2))
                break;
        }
        if (used == 0)
            break;

        linebuf[used] = 0;
        EPD_ShowChinese(text_x, cur_y, (u8 *)linebuf, FONT_SIZE_16, COLOR_BLACK);
        cur_y += line_height;

        if (p == start)
            break;
    }

    /* 右侧滚动条 */
    u16 scroll_x = SCREEN_H - 7;
    EPD_DrawRectangle(scroll_x, 48, scroll_x + 3, SCREEN_W - 12, COLOR_BLACK, 0);
    u16 slider_total_h = SCREEN_W - 48 - 7;
    u16 slider_h = (u16)(slider_total_h * 0.2f);
    if (slider_h < 12)
        slider_h = 12;
    u16 slider_pos = 48 + (u16)((slider_total_h - slider_h) * progress / 100);
    EPD_DrawRectangle(scroll_x + 1, slider_pos, scroll_x + 3, slider_pos + slider_h, COLOR_BLACK, 1);

    /* 底部文本（页码/百分比） */
    char bottom[32];
    snprintf(bottom, sizeof(bottom), "%d% %", progress);
    EPD_ShowString(8, SCREEN_W - 14, (u8 *)bottom, FONT_SIZE_12, COLOR_BLACK);
}

/**
 * @brief  计算一页小说能显示的最大字符数
 * @param  content   小说内容指针
 * @param  font_size 字体大小（与 GetStringWidth 匹配）
 * @return 当前页可显示的字符数（字节数）
 */
uint32_t UI_CalcReaderPageChars(const char *content, uint8_t font_size)
{
    const char *p = content;
    u16 cur_y = 44;                        // 起始Y坐标（同绘制函数）
    const u16 line_height = font_size + 4; // 行高
    char linebuf[256];
    uint32_t total_used = 0; // 本页累计显示的字节数
    u32 line_no = 0;

    // LOGD("UI_CalcReaderPageChars start: font_size=%u\n", (unsigned)font_size);

    while (*p && (cur_y + line_height <= SCREEN_W - 10))
    {
        const char *start = p;
        int used = 0;

        // 检查换行符
        if (*p == '\r' || *p == '\n')
        {
            if (*p == '\r' && *(p + 1) == '\n')
                p += 2;
            else
                p += 1;
            cur_y += line_height;
            total_used += 1; // 统计字节数（可选）
            continue;
        }

        // 每行按字节累加（注意：若为多字节编码应改为按字符）
        while (*p)
        {
            // 判断字符是否是多字节（这里简单假设 GBK 中文为两字节）
            if (*p == '\r' || *p == '\n')
                break;

            uint8_t c = (uint8_t)*p;
            int char_len = (c >= 0x81 && c <= 0xFE) ? 2 : 1;

            if (char_len == 2)
            {
                linebuf[used] = *p;
                linebuf[used + 1] = *(p + 1);
                linebuf[used + 2] = 0;
            }
            else
            {
                linebuf[used] = *p;
                linebuf[used + 1] = 0;
            }

            u16 tw = GetStringWidth(linebuf, font_size);
            if (tw > (SCREEN_H - 15))
            {
                if (char_len == 2)
                {
                    linebuf[used] = 0;
                    linebuf[used + 1] = 0;
                }
                else
                {
                    linebuf[used] = 0;
                }
                break;
            }

            used += char_len;
            p += char_len;
            total_used++; // 实现字数累加器
            if (used >= (int)(sizeof(linebuf) - 2))
                break;
        }

        if (used == 0)
        {
            LOGD("Line %lu: used==0 at cur_y=%u, p points to 0x%02X\r\n",
                 (unsigned long)line_no, (unsigned)cur_y, (unsigned char)*p);
            break; // 无可绘制字符，退出
        }

        // // 打印该行调试信息：字节数、像素宽度、内容摘取（可见字符）
        // {
        //     u16 tw_line = GetStringWidth(linebuf, font_size);
        //     char vis[64];
        //     int i, n = (used < 60) ? used : 60;
        //     for (i = 0; i < n; ++i)
        //     {
        //         char c = linebuf[i];
        //         vis[i] = (c >= 0x20 && c < 0x7F) ? c : '.';
        //     }
        //     vis[n] = 0;
        //     // LOGD("Line %lu: bytes=%d, pixel_w=%u, text=\"%s\"\r\n",
        //     //      (unsigned long)line_no, used, (unsigned)tw_line, vis);
        // }

        cur_y += line_height;
        // total_used += used;
        line_no++;

        if (p == start)
        {
            LOGD("No progress in pointer advance; breaking to avoid infinite loop.\r\n");
            break;
        }
    }

    // LOGD("UI_CalcReaderPageChars end: total_used=%lu, lines=%lu\r\n",
    //      (unsigned long)total_used, (unsigned long)line_no);
    return total_used;
}
// uint32_t UI_CalcReaderPageChars(const char *content, uint8_t font_size)
// {
//     const char *p = content;
//     u16 cur_y = 44;                            // 起始Y坐标
//     const u16 line_height = font_size + 4;     // 行高
//     char linebuf[256];
//     uint32_t total_used = 0;                   // 累积本页显示的字符数

//     while (*p && (cur_y + line_height <= SCREEN_W - 20))
//     {
//         const char *start = p;
//         int used = 0;

//         while (*p)
//         {
//             // 累加字符形成一行，用于计算像素宽度
//             linebuf[used] = *p;
//             linebuf[used + 1] = 0;

//             // 估算该行宽度
//             u16 tw = GetStringWidth(linebuf, font_size);

//             // 到达右边界则回退一个字符
//             if (tw > (SCREEN_H - 15))
//             {
//                 linebuf[used] = 0;
//                 break;
//             }

//             used++;
//             p++;

//             if (used >= (int)(sizeof(linebuf) - 2))
//                 break;
//         }

//         if (used == 0)
//             break;  // 无可绘制字符则退出

//         cur_y += line_height;
//         total_used += used;   // 累计显示字符数

//         if (p == start)
//             break;  // 防止死循环
//     }

//     return total_used;
// }
/**
 * @brief  计算一页小说能显示的最大字节数
 * @param  content   小说内容指针
 * @param  font_size 字体大小（与 GetStringWidth 匹配）
 * @return 当前页可显示的最大字节数
 */
#define SHOW_NEXT_TEXT 0
uint32_t UI_CalcReaderPageBytes(const char *content, uint8_t font_size)
{
    const char *p = content;
    const char *page_end = content;
    u16 cur_y = 44;
    const u16 line_height = font_size + 4;
    char linebuf[256];

    // 判断当前第一个字符是否是被偏移的GBK
    // if (*p)
    // {
    //     if (*p <= 0xFE)
    //     {
    //         // 这里这个代表是异常偏移错误的，第一个字节是半个字符的
    //         p++;
    //         flag_first_bytes_is_gbk_error = 1;
    //     }
    // }

    while (*p && (cur_y + line_height <= SCREEN_W - 10))
    {
        const char *start = p;
        int used = 0;

        if (*p == '\r' || *p == '\n')
        {
            if (*p == '\r' && *(p + 1) == '\n')
                p += 2;
            else
                p += 1;
            cur_y += line_height;
            page_end = p;
            continue;
        }

        while (*p)
        {
            if (*p == '\r' || *p == '\n')
                break;

            uint8_t c = (uint8_t)*p;
            int char_len = (c >= 0x81 && c <= 0xFE) ? 2 : 1;
            if (used + char_len >= sizeof(linebuf) - 2)
                break;

            memcpy(&linebuf[used], p, char_len);
            linebuf[used + char_len] = 0;

            u16 tw = GetStringWidth(linebuf, font_size);
            if (tw > (SCREEN_H - 15))
                break;

            used += char_len;
            p += char_len;
        }

        if (used == 0)
            break;

        cur_y += line_height;
        page_end = p;
    }
#if SHOW_NEXT_TEXT
    LOGD("显示余文本 =%s", page_end);
#endif
    // LOGD("显示余文本 =%s",page_end);
    return (uint32_t)(page_end - content);
}
// uint32_t UI_CalcReaderPageBytes(const char *content, uint8_t font_size)
// {
//     const char *p = content;
//     u16 cur_y = 44;                        // 起始Y坐标（同绘制函数）
//     const u16 line_height = font_size + 4; // 行高
//     char linebuf[256];
//     u32 line_no = 0;

//     while (*p && (cur_y + line_height <= SCREEN_W - 20))
//     {
//         const char *start = p;
//         int used = 0;

//         // 检查换行符
//         if (*p == '\r' || *p == '\n')
//         {
//             if (*p == '\r' && *(p + 1) == '\n')
//                 p += 2;
//             else
//                 p += 1;
//             cur_y += line_height;
//             continue;
//         }

//         // 按字节计算一行
//         while (*p)
//         {
//             if (*p == '\r' || *p == '\n')
//                 break;

//             uint8_t c = (uint8_t)*p;
//             int char_len = (c >= 0x81 && c <= 0xFE) ? 2 : 1;

//             // 复制到临时缓冲区（用于宽度计算）
//             if (c >= 0x81 && c <= 0xFE)
//             {
//                 linebuf[used] = *p;
//                 linebuf[used + 1] = *(p + 1);
//                 linebuf[used + 2] = 0;
//             }
//             else
//             {
//                 linebuf[used] = *p;
//                 linebuf[used + 1] = 0;
//             }

//             u16 tw = GetStringWidth(linebuf, font_size);
//             if (tw > (SCREEN_H - 15))
//             {
//                 linebuf[used] = 0;
//                 break;
//             }

//             used += char_len;
//             p += char_len;
//             if (used >= (int)(sizeof(linebuf) - 2))
//                 break;
//         }

//         if (used == 0)
//         {
//             LOGD("Line %lu: used==0 at cur_y=%u, p points to 0x%02X\r\n",
//                  (unsigned long)line_no, (unsigned)cur_y, (unsigned char)*p);
//             break;
//         }

//         cur_y += line_height;
//         line_no++;

//         if (p == start)
//         {
//             LOGD("No progress in pointer advance; breaking to avoid infinite loop.\r\n");
//             break;
//         }
//     }

//     // 返回本页实际使用的字节数
//     return (uint32_t)(p - content);
// }

#define LIST_START_X 10
#define LIST_START_Y 60
#define LIST_ITEM_H 24
#define LIST_WIDTH (SCREEN_W - 20)
#define LIST_VISIBLE_COUNT 6

void UI_DrawNovelListBox(const NovelListBox_t *box)
{
    Paint_NewImage(g_image_buf, SCREEN_W, SCREEN_H, 0, COLOR_WHITE);
    Paint_Clear(COLOR_WHITE);

    // 背景框
    EPD_DrawRectangle(0, 0, SCREEN_H - 1, SCREEN_W - 1, COLOR_BLACK, 0);

    // 顶部时间
    EPD_ShowString(5, 5, (uint8_t *)box->time_str, 16, COLOR_BLACK);

    // 标题
    EPD_ShowChinese(5, 25, (uint8_t *)"书架列表", 24, COLOR_BLACK);
    EPD_ShowChinese(5 + 100, 25 + 12, (uint8_t *)"(按 ↑ ↓ 按键翻页，按OK键选择)", 12, COLOR_BLACK);

    // 列表起始显示索引
    uint16_t start_index = 0;
    if (box->current_index >= LIST_VISIBLE_COUNT)
        start_index = box->current_index - (LIST_VISIBLE_COUNT / 2);

    if (start_index + LIST_VISIBLE_COUNT > box->novel_count)
        start_index = (box->novel_count > LIST_VISIBLE_COUNT) ? (box->novel_count - LIST_VISIBLE_COUNT) : 0;

    // 绘制列表项
    for (uint16_t i = 0; i < LIST_VISIBLE_COUNT; i++)
    {
        uint16_t idx = start_index + i;
        if (idx >= box->novel_count)
            break;

        uint16_t y = LIST_START_Y + i * LIST_ITEM_H;

        if (idx == box->current_index)
        {
            // 选中项背景高亮
            EPD_DrawRectangle(2, y - 2, SCREEN_H - 3, y + LIST_ITEM_H - 2, COLOR_BLACK, 1);
            EPD_ShowChinese(8, y, (uint8_t *)box->novel_list[idx], 16, COLOR_WHITE);
        }
        else
        {
            EPD_ShowChinese(8, y, (uint8_t *)box->novel_list[idx], 16, COLOR_BLACK);
        }
    }

    // 滚动指示和当前页进度展示
    if (box->novel_count > LIST_VISIBLE_COUNT)
    {
        if (start_index > 0)
            EPD_ShowChinese(SCREEN_H - 20, LIST_START_Y + LIST_VISIBLE_COUNT * LIST_ITEM_H + 15, (uint8_t *)"↑", 16, COLOR_BLACK);
        if (start_index + LIST_VISIBLE_COUNT < box->novel_count)
            EPD_ShowChinese(SCREEN_H - 20, LIST_START_Y + LIST_VISIBLE_COUNT * LIST_ITEM_H + 15, (uint8_t *)"↓", 16, COLOR_BLACK);
    }
    uint16_t totalPage = (box->novel_count / LIST_VISIBLE_COUNT) + (box->novel_count % LIST_VISIBLE_COUNT ? 1 : 0);
    uint16_t nowPage = (box->current_index / LIST_VISIBLE_COUNT) + 1;
    uint8_t pageStr[20];
    snprintf(pageStr, sizeof(pageStr), "第%d页 共%d页", nowPage, totalPage);
    // 尝试绘制字符到墨水屏
    EPD_ShowChinese(8, LIST_START_Y + LIST_VISIBLE_COUNT * LIST_ITEM_H + 15, pageStr, 16, COLOR_BLACK);
}

/* 将缓冲发送到屏幕（一次性显示当前 g_image_buf）*/
void UI_Show(void)
{
    EPD_FastInit();
    EPD_Display(g_image_buf);
    EPD_Update();
    EPD_DeepSleep();
}

void UI_PartShowInit(void)
{
    EPD_FastInit();
    EPD_Display_Clear();
    EPD_Update(); // 局刷之前先对E-Paper进行清屏操作
}
void UI_PartShow(void)
{
    if (partUpdateCounter >= 30)
    {
        // 调用全局刷新
        EPD_Init();
        EPD_Display(g_image_buf);
        EPD_Update();
        EPD_DeepSleep();
        partUpdateCounter = 0;
        // 调用完成后初始化局部刷新
        UI_PartShowInit();
        // 重置计数器
        // partUpdateCounter = 0 ;
    }
    EPD_PartInit();
    EPD_Display(g_image_buf);
    EPD_Update();        // 刷新墨水屏
    partUpdateCounter++; // 累加计数器
}

void UI_DrawErrorScreen(const char *error_msg)
{
    Paint_NewImage(g_image_buf, SCREEN_W, SCREEN_H, 0, COLOR_WHITE);
    Paint_Clear(COLOR_WHITE);
    EPD_DrawRectangle(0, 0, SCREEN_H - 1, SCREEN_W - 1, COLOR_BLACK, 0);

    /* 错误图标（居中）*/
    u16 icon_w = 80;
    u16 icon_x = (SCREEN_H - icon_w) / 2;
    u16 icon_y = 60;
    // EPD_DrawRectangle(icon_x, icon_y, icon_x + icon_w, icon_y + icon_w, COLOR_BLACK, 0);
    // 显示 :(  简笔画
    EPD_ShowString(icon_x + 25, icon_y + 30, (u8 *)":(", FONT_SIZE_24, COLOR_BLACK);

    if (error_msg)
        DrawCenteredString(icon_y + icon_w + 24, "Opps :", FONT_SIZE_16, COLOR_BLACK);
    DrawCenteredString(icon_y + icon_w + 48, error_msg, FONT_SIZE_12, COLOR_BLACK);

    // 打印提示
    DrawCenteredString(SCREEN_H - 30, "Please contact support, try restarting the device", FONT_SIZE_12, COLOR_BLACK);
}

void UI_DrawIndexLoadingScreen(uint32_t currentProgress, uint32_t totalProgress, uint8_t finish_flags)
{
    Paint_NewImage(g_image_buf, SCREEN_W, SCREEN_H, 0, COLOR_WHITE);
    Paint_Clear(COLOR_WHITE);
    EPD_DrawRectangle(0, 0, SCREEN_H - 1, SCREEN_W - 1, COLOR_BLACK, 0);

    /* 错误图标（居中）*/
    u16 icon_w = 80;
    u16 icon_x = (SCREEN_H - icon_w) / 2;
    u16 icon_y = 60;
    // EPD_DrawRectangle(icon_x, icon_y, icon_x + icon_w, icon_y + icon_w, COLOR_BLACK, 0);
    // 显示简笔画
    DrawCenteredString(icon_y + 30, (char *)finish_flags ? ":)" : "#", FONT_SIZE_24, COLOR_BLACK);

    if (finish_flags)
    {
        char progressPercentStr[32];
        snprintf(progressPercentStr, sizeof(progressPercentStr), "计算完毕");
        DrawCenteredString(icon_y + icon_w + 48, progressPercentStr, FONT_SIZE_12, COLOR_BLACK);
        // 打印提示
        DrawCenteredString(SCREEN_H - 30, "索引计算成功，请你重新设备！", FONT_SIZE_12, COLOR_BLACK);
    }
    else
    {
        DrawCenteredString(icon_y + icon_w + 24, "小说索引加载中 :", FONT_SIZE_16, COLOR_BLACK);
        // 计算进度百分比
        uint8_t progressPercent = currentProgress * 100 / totalProgress;
        char progressPercentStr[32];
        snprintf(progressPercentStr, sizeof(progressPercentStr), "已完成：%d%%", progressPercent);
        DrawCenteredString(icon_y + icon_w + 48, progressPercentStr, FONT_SIZE_12, COLOR_BLACK);
        // 打印提示
        DrawCenteredString(SCREEN_H - 30, "第一次打开小说，建立索引中", FONT_SIZE_12, COLOR_BLACK);
    }
}

// ======================  以下为 UI 局部跟新函数 ====================== */
void UI_flushTime_LockScreen(const RtcTimeType_t *time)
{
    /* 边框 */
    EPD_DrawRectangle(2, 2, SCREEN_H - 3, SCREEN_W - 3, COLOR_BLACK, 0);

    /* 时间（HH:MM） — 居中显示，采用大字号 24 */
    char timestr[16], date_str[20];
    snprintf(timestr, sizeof(timestr), "%02d:%02d", time->hour, time->minute);
    snprintf(date_str, sizeof(date_str), "20%02d年%02d月%02d日 周%s", time->year, time->month, time->date, weekend_str[Get_Weekday(time->year, time->month, time->date)]);
    // DrawCenteredTime(70, timestr, 48, COLOR_BLACK);
    uint16_t text_w = GetStringWidth(timestr, 48);
    EPD_DrawRectangle(124,33,124+text_w,33+48,COLOR_WHITE,1); // 擦除旧时间
    EPD_ShowString(124, 33, (u8 *)timestr, 48, COLOR_BLACK);
    /* 日期行 */
    if (date_str)
    {
        text_w = GetStringWidth(date_str, FONT_SIZE_16);
        EPD_DrawRectangle(84,91,84+text_w,91+16,COLOR_WHITE,1); // 擦除旧时间
        EPD_ShowChinese(84, 91, date_str, FONT_SIZE_16, COLOR_BLACK);
        // DrawCenteredString(130, date_str, FONT_SIZE_16, COLOR_BLACK);
    }
}

void UI_flushTime_NovelListBox(const RtcTimeType_t *time) {
    
}