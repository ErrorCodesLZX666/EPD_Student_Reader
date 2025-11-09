#include "gd32f4xx.h"
#include "systick.h"
#include <stdio.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "Usart0.h"
#include "log.h"
#include "spi.h"
#include "malloc.h"
#include "ff.h"
#include "sdcard.h"
#include "string.h"
#include "fat_app.h"
#include "systick.h"
#include "EPD.h"
#include "Pic.h"
#include "semphr.h"
#include "EPD_GUI.h"
#include "fontupd.h"
#include "APP_GUI.h"
#include "SYS_GUI.h"
#include "SYS_KEY.h"

// 尝试定义任务句柄
TaskHandle_t hTask_entries = NULL;
TaskHandle_t hTask_sys_init = NULL;
TaskHandle_t hTask_disk_montor_progress = NULL;
TaskHandle_t hTask_gui_config = NULL;
TaskHandle_t hTask_task_font_config = NULL;

// 磁盘监视器
void task_disk_montor(void *taskParam)
{

    while (1)
    {
        DiskInfo_t flash_disk = {}, sd_disk = {};
        getDiskInfo(&flash_disk, &sd_disk);
        LOGI("Disk current status: \n \tFLASH_MOUNT_ST = %d; \n \tSDIO_MOUNT_ST = %d", flash_disk.mounted, sd_disk.mounted);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}





typedef struct {
    uint32_t total_bytes;
    uint32_t total_pages;
    uint32_t current_page;
    uint32_t current_bytes;
} TestIndex;

int test_save_file(void)
{
    FRESULT res;
    FIL file;
    UINT bw;
    const char *test_path = "0:/Index/test.idx";
    DIR dir;

    LOGI("==== [TEST] SD File Write Test Start ====\r\n");

    // 确保目录存在
    res = f_opendir(&dir, "0:/Index");
    if (res != FR_OK) {
        LOGW("Directory 0:/Index not found (res=%d), creating...\r\n", res);
        res = f_mkdir("0:/Index");
        if (res != FR_OK) {
            LOGE("Create directory failed (res=%d)\r\n", res);
            return -1;
        }
    } else {
        f_closedir(&dir);
    }

    // 打开文件
    res = f_open(&file, test_path, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        LOGE("f_open failed: %s (res=%d)\r\n", test_path, res);
        return -2;
    }
    LOGI("File opened successfully: %s\r\n", test_path);

    // 准备要写入的测试数据（16 字节）
    TestIndex test_data = {
        .total_bytes = 1075739,
        .total_pages = 2693,
        .current_page = 120,
        .current_bytes = 48000
    };

    // 写入数据
    res = f_write(&file, &test_data, sizeof(TestIndex), &bw);
    LOGI("f_write result: res=%d, bw=%u (expect=%u)\r\n", res, bw, (unsigned)sizeof(TestIndex));

    if (res != FR_OK || bw != sizeof(TestIndex)) {
        LOGE("Write failed or incomplete.\r\n");
        f_close(&file);
        return -3;
    }

    // 强制同步
    res = f_sync(&file);
    LOGI("f_sync result: %d\r\n", res);

    // 关闭文件
    f_close(&file);
    LOGI("File closed successfully.\r\n");

    // 再次打开读取验证
    TestIndex readback;
    memset(&readback, 0, sizeof(readback));

    res = f_open(&file, test_path, FA_READ);
    if (res != FR_OK) {
        LOGE("Reopen file failed: %s (%d)\r\n", test_path, res);
        return -4;
    }

    UINT br;
    res = f_read(&file, &readback, sizeof(TestIndex), &br);
    f_close(&file);
    LOGI("f_read result: res=%d, br=%u\r\n", res, br);

    if (memcmp(&readback, &test_data, sizeof(TestIndex)) == 0) {
        LOGI("SD card write/read test passed. File system OK.\r\n");
        return 0;
    } else {
        LOGE("Data mismatch! SD card or FatFs error.\r\n");
        return -5;
    }
}


void task_force_update_font(void *taskParam)
{
    // 强制运行更新字库
    if (!update_font(0, 0, 16, "0:"))
    {
        // 字库更新成功
        LOGD("FONT loading successful!!!\r\n");
        // FONT_loading_success_flag = 1;
    }
    else
    {
        // 字库更新失败
        LOGE("FONT loading failed ... \r\n");
        // FONT_loading_success_flag = 0;
    }
}
// u8 ImageBuffergg[(416 * 240) / 8];
// 尝试初始化字库
void task_font_config()
{
    // 先看磁盘是否挂载成功
    if (!disk_sd.mounted || !disk_spi.mounted)
    {
        UI_DrawErrorScreen("No SDCARD/FlashIC fond");
        UI_PartShowInit();
        UI_PartShow();
        return;
    }
    
    // 首先尝试判断字库是否存在
    if (font_init())
    {
        // 字库有问题，尝试更新字库
        if (!update_font(0, 0, 16, "0:"))
        {
            // 字库更新成功
            LOGD("FONT loading successful!!!\r\n");
            // FONT_loading_success_flag = 1;
        }
        else
        {
            // 字库更新失败
            LOGE("FONT loading failed ... \r\n");

            UI_DrawErrorScreen("Font library load failed!");
            UI_PartShowInit();
            UI_PartShow();
            return;
            // FONT_loading_success_flag = 0;
        }
    }
    else
        LOGD("FONT loading successful !!!\r\n");

    // 最终在这里加载
    EPD_GPIOInit(); // 加载墨水屏驱动

    // 加载开机界面

    // UI_DrawLoadingScreen("智能学生便携墨水屏", "仅仅做懂学生的墨水屏，让墨水屏更便捷实用");
    // UI_Show();              // 使用驱动展示到墨水屏
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // // 加载使用锁屏界面
    // UI_DrawLockScreen(12,00,2025,11,1);
    // UI_PartShowInit();
    // UI_PartShow();
    // uint8_t min = 0;
    // for (min = 0; min < 60; min++)
    // {
    //     /* 测试是否是因为字库导致的 */
    //     min ++;
    //     UI_UpdateLockDatetime(12,min,2025,11,1);
    //     LOGD("Test Image buffer update ... index = %d \r\n",min);
    // }
    // while(1) {
    // 	min ++;
    //     // if(min % 30 == 0)   UI_DrawLockScreen(12,min,2025,11,1);
    //     vTaskDelay(pdMS_TO_TICKS(3000));
    //     UI_UpdateLockDatetime(12,min,2025,11,1);
    //     UI_PartShow();

    // }

    // char **txt_files = NULL;
    // int file_count = 0;
    // 尝试调用文件列表读取函数
    // sd_read_dir_txt("0:/", &txt_files, &file_count);
    // NovelListBox_t box = {
    //     .novel_list = txt_files,
    //     .novel_count = file_count,
    //     .current_index = 0,
    //     .time_str = "12:35"};

    // UI_DrawNovelListBox(&box);
    // UI_PartShowInit();
    // UI_PartShow();

    // 初始化项目级别的GUI
    GUI_key_config();
    SYSGUI_Entries(); // 启动GUI任务
    xTaskCreate(Key_Task, "Key_Task", 256, NULL, 3, NULL);


    // 下方代码是用来测试小说显示的
    // char test_text[] = "PS：\r\n①1V1主受HE。谢绝转载。\r\n②本文主线夫夫携手打怪解谜打孩子，前世今生双线剧情向。\r\n②nTest显示English本文主线夫夫携手打怪解谜打孩子，前世今生双线剧情向。\r\n③非复仇流！非升级流爽文！\r\n\r\n\r\n内容标签：重生 天作之合 灵异神怪 仙侠修真\r\n";
    // UI_DrawReaderPage("测试显示中文.txt","000",test_text,23);
    // // 尝试计算使用显示字节多少
    // uint32_t used_bytes = UI_CalcReaderPageBytes(test_text,16);
    // UI_PartShowInit();
    // UI_PartShow();
    // LOGD("统计完毕，显式当前需要使用到字节： %d\r\n",used_bytes);

    // test_save_file();

    while (1);
}

// 任务系统初始化
void task_sys_init(void *taskParam)
{
    BaseType_t ret;
    LOGD("SYS_PERH_INIT...\r\n");
    LOGD("SPI_INIT...\r\n");
    spi_config(); // 初始化SPI硬件通信
    LOGD("MYMEM_INIT...\r\n");
    my_mem_init(SRAMIN); // 初始化MCU内部内存
    LOGD("Fat32 FS INIT...\r\n");
    exfuns_init(); // 分配Fatfs内存
    LOGD("Mounting Disk [SDCARD]...\r\n");
    sd_fatfs_init(); // 初始化挂载SDCard磁盘
    LOGD("Mounting Disk [FLASH_SPI]...\r\n");
    spi_fatfs_init(); // 初始化挂载SPI_FLASH磁盘
    LOGD("Starting disk mountor progreess...\r\n");
    xTaskCreate(task_disk_montor, "task_disk_montor", 1024, NULL, 1, &hTask_disk_montor_progress);
    LOGD("Starting FONT init progress ...\r\n");
    // 启动磁盘监听
    printf("[Staring done]\r\n");
    LOGW("Trying to loading GUI ...\r\n");
    ret = xTaskCreate(task_font_config, "task_font_config", 1024, NULL, 1, &hTask_task_font_config);
    // ret = xTaskCreate(task_force_update_font,"task_force_update_font",1024,NULL,1,NULL);

    // 尝试启动LVGL
    while (1);
}

// 任务入口点
void task_entries(void *taskParm)
{
    BaseType_t ret;
    // 尝试启用其他的任务
    ret = xTaskCreate(task_sys_init, "task_sys_init", 1024, NULL, 1, &hTask_sys_init);
    if (ret != pdPASS)
        LOGE("Task start Failed ... \r\n");
    // 最终尝试结束本任务
    vTaskDelete(NULL);
}

int main(void)
{
    // systick_config();
    usart0_init();
    log_sema_init(); // 创建日志互斥体
    printf("------------- SYSTEM START UP -------------\r\n");
    printf("BOOTLOADER Loading successfull ...\r\n");
    printf("SYSTEMOS: PDE Portal Student Reader \r\n");
    printf("HARDWARE VER: Beta 0.1");
    printf("Copyright LeiZhiXiang (C) All rights recived...\r\n");
    printf("-------------------------------------------\r\n");
    // vTaskDelay(pdMS_TO_TICKS(500));
    //  执行任务操作
    xTaskCreate(task_entries, "task_entries", 1024 * 10, NULL, 1, &hTask_entries);
    // 开启任务调度功能
    vTaskStartScheduler();
    while (1)
        ;
}

void usart0_on_recive(uint8_t *datas, uint8_t len)
{
    // TODOS
    printf("%s", datas);
}