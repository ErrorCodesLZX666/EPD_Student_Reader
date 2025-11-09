#include "SYS_KEY.h"
#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"
#include "GUI_EVENT.h"
#include "log.h"

// 导入 RTOS 的库函数
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define KEY_SCAN_INTERVAL 50 // ms，去抖周期

void GUI_key_config(void)
{
    // 初始化当前硬件GPIO，都是属于输入模式
    rcu_periph_clock_enable(KEY_PAGE_UP_GPIO_RCU);
    rcu_periph_clock_enable(KEY_PAGE_DOWN_GPIO_RCU);
    rcu_periph_clock_enable(KEY_OK_GPIO_RCU);
    rcu_periph_clock_enable(KEY_PAGE_UP_GPIO_RCU);
    rcu_periph_clock_enable(KEY_HOME_GPIO_RCU);

    gpio_mode_set(KEY_PAGE_UP_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KEY_PAGE_UP_GPIO_PIN);
    gpio_mode_set(KEY_PAGE_DOWN_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KEY_PAGE_DOWN_GPIO_PIN);
    gpio_mode_set(KEY_OK_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KEY_OK_GPIO_PIN);
    gpio_mode_set(KEY_CANCEL_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KEY_CANCEL_GPIO_PIN);
    gpio_mode_set(KEY_HOME_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KEY_HOME_GPIO_PIN);
}

static KeyCode_t Key_Scan(void)
{
    if (gpio_input_bit_get(KEY_PAGE_UP_GPIO_PORT, KEY_PAGE_UP_GPIO_PIN) == SET)
        return KEY_UP;
    if (gpio_input_bit_get(KEY_PAGE_DOWN_GPIO_PORT, KEY_PAGE_DOWN_GPIO_PIN) == SET)
        return KEY_DOWN;
    if (gpio_input_bit_get(KEY_OK_GPIO_PORT, KEY_OK_GPIO_PIN) == SET)
        return KEY_OK;
    if (gpio_input_bit_get(KEY_CANCEL_GPIO_PORT, KEY_CANCEL_GPIO_PIN) == SET)
        return KEY_CANCEL;
    if (gpio_input_bit_get(KEY_HOME_GPIO_PORT, KEY_HOME_GPIO_PIN) == SET)
        return KEY_HOME;
    return KEY_NONE;
}

void Key_Task(void *param)
{
    (void)param;
    KeyCode_t lastKey = KEY_NONE;
    for (;;)
    {
        KeyCode_t key = Key_Scan();
        // 
        if (key != lastKey && key != KEY_NONE)
        {
            LOGI("Key touched, key == %d", key, KEY_HOME);
            GUI_Event_t event = {.type = GUI_EVENT_KEY, .key = key};
            xQueueSend(guiEventQueue, &event, 0);
        }
        lastKey = key;
        vTaskDelay(pdMS_TO_TICKS(KEY_SCAN_INTERVAL));
    }
}