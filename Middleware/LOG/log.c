#include "log.h"
// 尝试导入 RTOS 中的任务互斥体，实现任务安全设计
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// 创建唯一的同步锁
SemaphoreHandle_t xMutex = NULL; // 互斥锁唯一对象
uint8_t useSemaLock = 0;         // 是否使用互斥锁

void log_sema_init()
{
    xMutex = xSemaphoreCreateMutex();
    useSemaLock = 1; // 使用互斥锁
}

void log_print(LogLevel level, const char *func, int line, const char *fmt, ...)
{
    const char *level_str;

    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        level_str = "DEBUG";
        break;
    case LOG_LEVEL_INFO:
        level_str = "INFO ";
        break;
    case LOG_LEVEL_WARN:
        level_str = "WARN ";
        break;
    case LOG_LEVEL_ERROR:
        level_str = "!!!ERROR!!!";
        break;
    default:
        level_str = "UNKWN";
        break;
    }
    // 判断是否拥有互斥锁
    if (useSemaLock)
    {
        // 执行互斥锁逻辑
        // 获取锁
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {

            // 输出头部信息：[LEVEL] [function :: line]
            printf("* [%s-%s :: %d] ", level_str, func, line);
            // 可变参数格式化输出
            va_list args;
            va_start(args, fmt);
            vprintf(fmt, args);
            va_end(args);

            printf("\n");
            // 释放同步锁
            xSemaphoreGive(xMutex);
        }
    }
    else
    {
        // 普通逻辑
        // 输出头部信息：[LEVEL] [function :: line]
        printf("[%s-%s :: %d] ", level_str, func, line);

        // 可变参数格式化输出
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);

        printf("\n");
    }
}
