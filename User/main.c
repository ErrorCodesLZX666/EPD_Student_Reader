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
#include "mcu_rtc.h"
#include "AHT20.h"

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

// 定义是否输出时间到屏幕的任务
#define SHOW_TIME_TO_SCREEN 1
void task_loop_monitor(void *taskParams)
{
    RtcTimeType_t time;
    uint8_t last_minute = 0;

    // AHT20 温湿度检测变量
    float temperature = 0;
    float humidity = 0;

    // 尝试输出时间
    while (1)
    {

        RtcGetCurrentTimeStruct(&time);
        AHT20_Detection_Start();
        vTaskDelay(pdMS_TO_TICKS(100)); // 等待测量完成
        AHT20_Get_Values(&temperature, &humidity);
#if SHOW_TIME_TO_SCREEN
        LOGD("TimeMonitor Current time: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
             time.year, time.month, time.date,
             time.hour, time.minute, time.second);
        LOGD("----------------------------------------\r\n");
        LOGD("AHT20Monitor Current Temperature: %.2f C, Humidity: %.2f %%\r\n", temperature, humidity);
        LOGD("----------------------------------------\r\n");

#endif
        if (last_minute != time.minute)
        {
            last_minute = time.minute;
            // 每隔一段时间保存一次时间,这里使用min来确认保存
            // TODOS： 调用GUI中的时间更新函数，尝试判断不同的界面并且尝试更新时间
            UI_TimeUpdateToScreen(&time);
            UI_AHT20UpdateToScreen(temperature, humidity);
            // 保存时间到Flash中
            save_sys_time(&time);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
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
    // 初始化项目级别的GUI
    GUI_key_config();
    SYSGUI_Entries(); // 启动GUI任务
    xTaskCreate(Key_Task, "Key_Task", 4096, NULL, 3, NULL);

    while (1)
        ;
}

// 任务系统初始化
void task_sys_init(void *taskParam)
{
	LOGD("current is loading printf ...\r\n");
	// vTaskDelay(pdMS_TO_TICKS(2000));
	LOGD("LOGD loading ... \r\n");
 
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
    // 初始化温湿度检测
    LOGD("AHT20 INIT...\r\n");
    // vTaskDelay(pdMS_TO_TICKS(1000));
    LOGD("AHT20 INIT..asdfadfasf.\r\n");
    AHT20_Init();
    LOGD("Loading RTC Clock ...\r\n");
    rtc_config(); // 初始化RTC时钟
    LOGD("Starting disk mountor progreess...\r\n");
    xTaskCreate(task_disk_montor, "task_disk_montor", 4096, NULL, 1, &hTask_disk_montor_progress);
    LOGD("Starting FONT init progress ...\r\n");
    // 启动磁盘监听
    printf("[Staring done]\r\n");
    LOGW("Trying to loading GUI ...\r\n");
    ret = xTaskCreate(task_font_config, "task_font_config", 4096, NULL, 1, &hTask_task_font_config);
    // ret = xTaskCreate(task_force_update_font,"task_force_update_font",4096,NULL,1,NULL);
    ret = xTaskCreate(task_loop_monitor, "task_loop_monitor", 4096, NULL, 1, NULL);
    // 尝试启动LVGL
    while (1)
        ;
}

// 任务入口点
void task_entries(void *taskParm)
{
<<<<<<< HEAD

=======
	  printf("start task running ...\r\n");
	
>>>>>>> EPD_Student_Reader/master
    BaseType_t ret;
	
	
    // 尝试启用其他的任务
    ret = xTaskCreate(task_sys_init, "task_sys_init", 8192, NULL, 1, &hTask_sys_init);
<<<<<<< HEAD

=======
	printf("start task running  666  ...\r\n");
>>>>>>> EPD_Student_Reader/master
    if (ret != pdPASS)
        LOGE("Task start Failed ... \r\n");
		printf("start task running finished  ...\r\n");
    // 最终尝试结束本任务
    vTaskDelete(NULL);
		//while(1) ;
}

void testDemo()
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        LOGD("test");
    }
}

int main(void)
{
    // systick_config();
    usart0_init();
    log_sema_init(); // 创建日志互斥体
//    printf("------------- SYSTEM START UP -------------\r\n");
//    printf("BOOTLOADER Loading successfull ...\r\n");
//    printf("SYSTEMOS: PDE Portal Student Reader \r\n");
//    printf("HARDWARE VER: Beta 0.1");
//    printf("Copyright LeiZhiXiang (C) All rights recived...\r\n");
//    printf("-------------------------------------------\r\n");

    //  执行任务操作
<<<<<<< HEAD
    xTaskCreate(task_entries, "task_entries", 1024 * 10, NULL, 1, &hTask_entries);
    // xTaskCreate(testDemo, "testDemo", 1024, NULL, 1, NULL);
    //  开启任务调度功能
    vTaskStartScheduler();
    while (1)
        ; // 不阻塞调度;
=======
    xTaskCreate(task_sys_init, "task_entries", 1024 * 10, NULL, 1, &hTask_entries);

	    // 开启任务调度功能
    vTaskStartScheduler();
        while (1) {
        // 在主循环中添加一些逻辑，避免死循环
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
>>>>>>> EPD_Student_Reader/master
}

void usart0_on_recive(uint8_t *datas, uint8_t len)
{
    // TODOS
<<<<<<< HEAD
    // printf("%s", datas);
=======
    printf("%s", datas);
}



void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* 输出哪个任务溢出了 */
    printf(">>> Stack Overflow! Task name: %s <<<\r\n", pcTaskName);

    /* 死循环，防止继续运行破坏系统 */
    taskDISABLE_INTERRUPTS();
    for(;;);
}


void vApplicationMallocFailedHook(void)
{
    printf(">>> Malloc Failed! FreeRTOS heap exhausted <<<\r\n");

    taskDISABLE_INTERRUPTS();
    for(;;);
>>>>>>> EPD_Student_Reader/master
}