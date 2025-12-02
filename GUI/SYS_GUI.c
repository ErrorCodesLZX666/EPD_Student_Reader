#include "SYS_GUI.h"
#include "EPD.h"
#include "EPD_GUI.h"
#include "APP_GUI.h"
#include "SYS_KEY.h"
#include "log.h"
#include "GUI_EVENT.h"
#include "malloc.h"
#include "fat_app.h"
#include <string.h>
#include "AHT20.h"

#include "FreeRTOS.h" // 加载RTOS
#include "task.h"

#include "ff.h"

uint8_t hour, minute, second, mouth, day; // RTC 时分秒月日
uint16_t year;                            // RTC 年

// 通过 RTC时钟获取时间，这里构建一个虚拟的时间
void looptask_get_current_time(void)
{
    while (1)
    {
        // TODOS: 这里是使用的默认数据，请使用RTC来进行替代
        hour = 11;
        minute = 53;
        second = 22;
        mouth = 5;
        day = 3;
        year = 2025;
        vTaskDelay(pdMS_TO_TICKS(1000)); // 一秒就更新时间
    }
}

UI_State_t uiStatus = UI_STATE_LOADING; // 默认开机界面

void UI_SwitchTo(UI_State_t newState)
{
    uiStatus = newState;
    UI_Refresh();
}

// 将值给定义到外面来
// ======== 全局静态数据 ========

// 全局小说框数据结构
NovelListBox_t box = {
    .novel_list = NULL,
    .novel_count = 0,
    .current_index = 0,
    .time_str = "12:35"};

// 尝试读取小说索引
NovelIndex novelIndex = {};
// ------- 小说阅读值
// 小说阅读界面数据
const char *novelText;
const char *novelProgress;
const char *novelContent;
uint16_t current_page_content_bytes = 0; // 翻页内容的字节数

void safe_read_page(const char *novelPath, uint32_t offset, char *buf, uint32_t size, uint32_t *read_len)
{
    // 为防止读取越界，提前处理
    uint32_t start = (offset > 0) ? offset - 1 : 0;

    // 临时缓冲，多读1字节
    char tmpbuf[513];
    sd_read_range(novelPath, start, start + size + 1, tmpbuf, size + 1, read_len);

    // 如果不是文件起始位置，判断是否在双字节中间
    if (offset > 0)
    {
        uint8_t prev = (uint8_t)tmpbuf[0];
        uint8_t first = (uint8_t)tmpbuf[1];
        // 如果前一个字节是GBK高字节（81–FE），而当前字节是尾字节（40–FE），说明被截断
        if ((prev >= 0x81 && prev <= 0xFE) &&
            (first >= 0x40 && first <= 0xFE) && (first != 0x7F))
        {
            LOGD("[AlignFix] Page start misaligned. Adjusting offset back by 1 byte (GBK boundary).\r\n");
            // 向前1字节才是合法字符边界
            start = offset - 1;
            sd_read_range(novelPath, start, start + size, buf, size, read_len);
            return;
        }
    }

    // 否则直接复制有效数据
    memcpy(buf, tmpbuf + (offset > 0 ? 1 : 0), size);
}

void UI_List_Page_down_action(void)
{
    LOGI("Pagedown touched , current_index = %d, now_index = %d", box.current_index, box.novel_count);
    if (box.novel_count > (box.current_index + 1))
    {
        box.current_index++;
        UI_DrawNovelListBox(&box);
        UI_PartShow();
    }
}
void UI_List_Page_up_action(void)
{
    LOGI("Pageup touched , current_index = %d, now_index = %d", box.current_index, box.novel_count);
    if ((box.current_index) > 0)
    {
        box.current_index--;
        UI_DrawNovelListBox(&box);
        UI_PartShow();
    }
}
void UI_Content_Page_up_action(void)
{
    char novelPath[64];
    char indexPath[64];
    // NovelIndex index;
    snprintf(indexPath, sizeof(indexPath), "0:/Index/%s.idx", novelText);
    snprintf(novelPath, sizeof(novelPath), "0:/%s", novelText);
    // load_novel_index(indexPath, &novelIndex);
    if (novelIndex.current_page > 0)
    {
        // 最新代码只用读取索引文件对应的字节即可
        // 定义本次读取的范围
        uint32_t read_start_at = novelIndex.current_bytes - novel_read_page_bytes(indexPath, novelIndex.current_page - 1);
        uint32_t read_end_at = novelIndex.current_bytes;
        // 读取小说内容
        sd_read_range(novelPath, read_start_at, read_end_at, (char *)novelContent, read_end_at - read_start_at, &current_page_content_bytes);
        LOGD("=========================\r\n");
        LOGD("Page read start offset: %lu bytes\r\n", read_start_at);
        LOGD("Page read end offset: %lu bytes\r\n", read_end_at);
        LOGD("Read bytes: %d bytes\r\n", current_page_content_bytes);
        LOGD("page - 1 bytes from index: %d bytes\r\n", novel_read_page_bytes(indexPath, novelIndex.current_page - 1));
        LOGD("page  bytes from index: %d bytes\r\n", novel_read_page_bytes(indexPath, novelIndex.current_page));
        LOGD("First few bytes: %02X %02X %02X %02X %02X\r\n",
             (unsigned char)novelContent[0], (unsigned char)novelContent[1],
             (unsigned char)novelContent[2], (unsigned char)novelContent[3],
             (unsigned char)novelContent[4]);
        LOGD("=========================\r\n");
        // 更新索引
        novelIndex.current_bytes -= novel_read_page_bytes(indexPath, novelIndex.current_page - 1);
        novelIndex.current_page--;
        // 显示小说内容
        uint8_t novelProgressText[32];
        snprintf((char *)novelProgressText, sizeof(novelProgressText), "第 %lu / %lu 页", (unsigned long)(novelIndex.current_page), (unsigned long)novelIndex.total_pages);
        UI_DrawReaderPage(novelText, novelProgressText, novelContent, (int)(novelIndex.current_page * 100 / novelIndex.total_pages));
        UI_PartShow();
        // 将上页的索引保存
        save_novel_index(indexPath, &novelIndex);
        // sd_read_range(novelPath,novelIndex.current_bytes - current_page_content_bytes > 512 ? novelIndex.current_bytes - current_page_content_bytes - 512 : novelIndex.current_bytes - current_page_content_bytes  , novelIndex.current_bytes - current_page_content_bytes, (char *)novelContent, novelIndex.current_bytes - 512 > 0 ? 512 : novelIndex.current_bytes, &current_page_content_bytes);
        // // safe_read_page(novelPath,index.current_bytes - 512 > 0 ? index.current_bytes - 512 : 0, (char *)novelContent, 512, &current_page_content_bytes);
        // UI_DrawReaderPage(novelText, novelProgress, novelContent, (int)(novelIndex.current_page * 100 / novelIndex.total_pages));
        // UI_PartShow();
        // LOGD("Page read offset: %lu bytes\r\n", novelIndex.current_bytes);
        // LOGD("First few bytes: %02X %02X %02X %02X %02X\r\n",
        //      (unsigned char)novelContent[0], (unsigned char)novelContent[1],
        //      (unsigned char)novelContent[2], (unsigned char)novelContent[3],
        //      (unsigned char)novelContent[4]);

        // // 尝试更新索引
        // current_page_content_bytes = UI_CalcReaderPageBytes(novelContent, FONT_SIZE_16);
        // LOGD("Current page content bytes: %d Step up...\r\n", current_page_content_bytes);
        // novelIndex.current_bytes -= current_page_content_bytes;
        // novelIndex.current_page -= 1;
        // // 最终保存索引
        // LOGD("Trying to save current novel index...\r\n");
    }
}
void UI_Content_Page_down_action(void)
{
    char novelPath[64];
    char indexPath[64];
    // NovelIndex index;
    snprintf(indexPath, sizeof(indexPath), "0:/Index/%s.idx", novelText);
    snprintf(novelPath, sizeof(novelPath), "0:/%s", novelText);
    // load_novel_index(indexPath, &novelIndex);
    // 判断是否已经越界
    if (novelIndex.current_page < novelIndex.total_pages)
    {

        // 最新代码只用读取索引文件对应的字节即可
        // 定义本次读取的范围
        uint32_t read_start_at = novelIndex.current_bytes + novel_read_page_bytes(indexPath, novelIndex.current_page);
        uint32_t read_end_at = read_start_at + novel_read_page_bytes(indexPath, novelIndex.current_page + 1);
        // 读取小说内容
        sd_read_range(novelPath, read_start_at, read_end_at, (char *)novelContent, read_end_at - read_start_at, &current_page_content_bytes);

        LOGD("=========================\r\n");
        LOGD("Page read start offset: %lu bytes\r\n", read_start_at);
        LOGD("Page read end offset: %lu bytes\r\n", read_end_at);
        LOGD("Read bytes: %d bytes\r\n", current_page_content_bytes);
        LOGD("page bytes from index: %d bytes\r\n", novel_read_page_bytes(indexPath, novelIndex.current_page));
        LOGD("page + 1 bytes from index: %d bytes\r\n", novel_read_page_bytes(indexPath, novelIndex.current_page + 1));
        LOGD("First few bytes: %02X %02X %02X %02X %02X\r\n",
             (unsigned char)novelContent[0], (unsigned char)novelContent[1],
             (unsigned char)novelContent[2], (unsigned char)novelContent[3],
             (unsigned char)novelContent[4]);
        LOGD("=========================\r\n");
        // 更新索引
        novelIndex.current_bytes += novel_read_page_bytes(indexPath, novelIndex.current_page);
        novelIndex.current_page++;
        // 显示小说内容
        uint8_t novelProgressText[32];
        snprintf((char *)novelProgressText, sizeof(novelProgressText), "第 %lu / %lu 页", (unsigned long)(novelIndex.current_page), (unsigned long)novelIndex.total_pages);
        UI_DrawReaderPage(novelText, novelProgressText, novelContent, (int)(novelIndex.current_page * 100 / novelIndex.total_pages));
        UI_PartShow();
        // 将上页的索引保存
        save_novel_index(indexPath, &novelIndex);

        // // 执行翻页操作逻辑
        // sd_read_range(novelPath, novelIndex.current_bytes, novelIndex.current_bytes + 512, (char *)novelContent, 512, &current_page_content_bytes);
        // // safe_read_page(novelPath, index.current_bytes, (char *)novelContent, 512, &current_page_content_bytes);
        // UI_DrawReaderPage(novelText, novelProgress, novelContent, (int)(novelIndex.current_page * 100 / novelIndex.total_pages));
        // UI_PartShow();

        // LOGD("Page read offset: %lu bytes\r\n", novelIndex.current_bytes);
        // LOGD("First few bytes: %02X %02X %02X %02X %02X\r\n",
        //      (unsigned char)novelContent[0], (unsigned char)novelContent[1],
        //      (unsigned char)novelContent[2], (unsigned char)novelContent[3],
        //      (unsigned char)novelContent[4]);
        // // 尝试更新索引
        // current_page_content_bytes = UI_CalcReaderPageBytes(novelContent, FONT_SIZE_16);
        // LOGD("Current page content bytes: %d Step down...\r\n", current_page_content_bytes);
        // novelIndex.current_bytes += current_page_content_bytes;
        // novelIndex.current_page += 1;
        // // 最终保存索引
        // LOGD("Trying to save current novel index...\r\n");
    }
}

void UI_Logic_get_novel_list(void)
{
    // 尝试使用文件系统来进行获取小说列表
}

void UI_Refresh(void)
{

    switch (uiStatus)
    {
    case UI_STATE_LOCK:
        RtcTimeType_t time;
        RtcGetCurrentTimeStruct(&time);
        UI_DrawLockScreen(time.hour, time.minute, time.year, time.month, time.date);
        AHT20_Detection_Start();
        vTaskDelay(pdMS_TO_TICKS(100)); // 等待测量完成
        float temperature = 0;
        float humidity = 0;
        AHT20_Get_Values(&temperature, &humidity);
        UI_flushAHT20_LockScreen(temperature, humidity);
        UI_PartShow();
        break;
    case UI_STATE_LOADING:
        UI_DrawLoadingScreen("智能学生便携墨水屏", "仅仅做懂学生的墨水屏，让墨水屏更便捷实用");
        UI_PartShow();
        break;
    case UI_STATE_LIST:
    {
        char **novelsList;
        // 在这里初始化小说列表
        sd_read_dir_txt("0:/", &novelsList, &box.novel_count);
        box.novel_list = novelsList;
        UI_DrawNovelListBox(&box);
        RtcTimeType_t time;
        RtcGetCurrentTimeStruct(&time);
        UI_flushTime_NovelListBox(&time);
        UI_PartShow();
        break;
    }
    case UI_STATE_READER:
        // 拼接小说路径
        char novelPath[64];
        char indexPath[64];
        char *novelfileName = box.novel_list[box.current_index];
        novelText = novelfileName;
        snprintf(novelPath, sizeof(novelPath), "0:/%s", novelfileName);
        snprintf(indexPath, sizeof(indexPath), "0:/Index/%s.idx", novelfileName);
        // 在这里读取小说内容，但是在开始以前得判断是否拥有索引
        if (box.current_index < box.novel_count)
        {
            // 判断索引文件是否存在
            FIL idxFile;
            if (f_open(&idxFile, indexPath, FA_READ) != FR_OK)
            {
                // 索引文件不存在，重新计算索引
                NovelIndex index = {};
                EPD_GPIOInit();
                EPD_Init();
                if (Novel_CalcIndex(novelPath, indexPath, FONT_SIZE_16, &index) == FR_OK)
                {
                    index.current_bytes = 0;
                    index.current_page = 0;
                    LOGE("Trying to write index file...\r\n");
                    // LOGE("请注意这里还没有保存文件！！！\r\n");
                    // 保存索引文件
                    if (save_novel_index(indexPath, &index))
                    {
                        LOGE("Failed to save index file: %s\r\n", indexPath);
                    }
                }
            }
            else
            {
                // 关闭文件
                f_close(&idxFile);
            }

            LOGI("trying to load novel index from %s\r\n", indexPath);
            load_novel_index(indexPath, &novelIndex);
            LOGI("Loaded novel index: total_bytes=%lu, total_pages=%lu\r\n",
                 (unsigned long)novelIndex.total_bytes,
                 (unsigned long)novelIndex.total_pages);
            // 读取索引中保存的内容
            novelContent = mymalloc(SRAMIN, 512);
            LOGI("Trying to read first page content...\r\n");
            sd_read_range(novelPath, novelIndex.current_bytes, novelIndex.current_bytes + 512, (char *)novelContent, 512, &current_page_content_bytes);
            uint8_t novelProgressText[32];
            snprintf((char *)novelProgressText, sizeof(novelProgressText), "第 %lu / %lu 页", (unsigned long)(novelIndex.current_page), (unsigned long)novelIndex.total_pages);
            UI_DrawReaderPage(novelfileName, novelProgressText, novelContent, (int)(novelIndex.current_page * 100 / novelIndex.total_pages));
            UI_PartShow();
            current_page_content_bytes = UI_CalcReaderPageBytes(novelContent, FONT_SIZE_16);
            // novelIndex.current_bytes += current_page_content_bytes;
            myfree(SRAMIN, novelContent);
		
        }
        break;
    }
}

/* GUI主任务 */
static void vTask_GUI(void *param)
{
    (void)param;
    UI_Refresh(); // 首次绘制模拟开机
    vTaskDelay(pdMS_TO_TICKS(3000));
    uiStatus = UI_STATE_LOCK;
    UI_Refresh();

    for (;;)
    {
        GUI_Event_t event;
        if (xQueueReceive(guiEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            GUI_HandleEvent(&event); // 统一分发
        }
    }
}

/* 入口函数 */
void SYSGUI_Entries(void)
{
    GUI_Event_Init();
    // 调用部分显示初始化函数
    EPD_GPIOInit();
    UI_PartShowInit();
    xTaskCreate(vTask_GUI, "GUI_Task", 2048, NULL, 2, NULL);
}

void UI_TimeUpdateToScreen(const RtcTimeType_t *time)
{
    // 使用Switch进行判断
    switch (uiStatus)
    {
    case UI_STATE_LOCK:
        // 等于锁屏界面
        UI_flushTime_LockScreen(time); 
        UI_PartShow();
        break;
    case UI_STATE_LIST:
        // 等于列表界面
        UI_flushTime_NovelListBox(time);
        UI_PartShow();
        break;
    default:
        LOGE("Unhandled UI state for time update,no thing to do. UI_STATUS: %d\r\n", uiStatus);
        break;
    }
}

void UI_AHT20UpdateToScreen(float temperature, float humidity)
{
    // 使用Switch进行判断
    switch (uiStatus)
    {
    case UI_STATE_LOCK:
        // 等于锁屏界面
        UI_flushAHT20_LockScreen(temperature, humidity);
        UI_PartShow();
        break;
    default:
        LOGE("Unhandled UI state for AHT20 update,no thing to do. UI_STATUS: %d\r\n", uiStatus);
        break;
    }
}