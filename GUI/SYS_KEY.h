#ifndef __SYS_KEY_H__
#define __SYS_KEY_H__
#include "gd32f4xx.h"

// 定义按钮硬件针脚
#define KEY_PAGE_UP_GPIO_PORT		GPIOE               // 翻上页
#define KEY_PAGE_UP_GPIO_PIN		GPIO_PIN_4	
#define KEY_PAGE_UP_GPIO_RCU		RCU_GPIOE

#define KEY_PAGE_DOWN_GPIO_PORT		GPIOE               // 翻下页
#define KEY_PAGE_DOWN_GPIO_PIN		GPIO_PIN_2	
#define KEY_PAGE_DOWN_GPIO_RCU		RCU_GPIOE

#define KEY_OK_GPIO_PORT		GPIOE                   // OK按键 
#define KEY_OK_GPIO_PIN		GPIO_PIN_6	
#define KEY_OK_GPIO_RCU		RCU_GPIOE

#define KEY_CANCEL_GPIO_PORT		GPIOE               // 取消按键
#define KEY_CANCEL_GPIO_PIN		GPIO_PIN_5	
#define KEY_CANCEL_GPIO_RCU		RCU_GPIOE

#define KEY_HOME_GPIO_PORT		GPIOC                   // HOME 复原按键
#define KEY_HOME_GPIO_PIN		GPIO_PIN_13	
#define KEY_HOME_GPIO_RCU		RCU_GPIOC

typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_OK,
    KEY_CANCEL,
    KEY_HOME
} KeyCode_t;

void GUI_key_config(void);
void Key_Task(void *param);  // FreeRTOS按键扫描任务

#endif