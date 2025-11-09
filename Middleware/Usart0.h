#ifndef __USAR0_H__
#define __USAR0_H__

#include "gd32f4xx.h"
#include "gd32f4xx_usart.h"
#include <stdio.h>
#include "systick.h"


// 定义初始化函数
void usart0_init();

// 定义数据输出函数
void usart0_send_byte(uint8_t data);				// 一次输出一个字节
void usart0_send_data(uint8_t *data, uint8_t len);	// 一次输出多个定长字节
void usart0_send_string(uint8_t *str);				// 一次输出一个字符串，注意字符串需要用 /00 来结尾。


// 预留输入回调函数
extern void usart0_on_recive(uint8_t *datas,uint8_t len);

#endif