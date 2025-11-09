#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdarg.h>

// ==================== 配置区 ====================

// 是否启用日志输出
#define LOG_ENABLE        1


void log_sema_init();

// 定义日志等级
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

// ==================== 接口函数 ====================

void log_print(LogLevel level, const char *func, int line, const char *fmt, ...);

// 定义方便使用的宏
#if LOG_ENABLE

    #define LOGD(fmt, ...) log_print(LOG_LEVEL_DEBUG, __func__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOGI(fmt, ...) log_print(LOG_LEVEL_INFO,  __func__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOGW(fmt, ...) log_print(LOG_LEVEL_WARN,  __func__, __LINE__, fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) log_print(LOG_LEVEL_ERROR, __func__, __LINE__, fmt, ##__VA_ARGS__)

#else
    #define LOGD(fmt, ...)
    #define LOGI(fmt, ...)
    #define LOGW(fmt, ...)
    #define LOGE(fmt, ...)
#endif

#endif
