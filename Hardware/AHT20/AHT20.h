#ifndef __AHT20_H
#define __AHT20_H

#include "gd32f4xx.h"
#include "systick.h"

//定义软件I2C延时
#define AHT20_I2C_Delay delay_1ms(1)

//@brief 定义软件I2C IO
#define AHT20_SCL_RCU  RCU_GPIOB
#define AHT20_SCL_PORT GPIOB
#define AHT20_SCL_PIN  GPIO_PIN_5

#define AHT20_SDA_RCU  RCU_GPIOB
#define AHT20_SDA_PORT GPIOB
#define AHT20_SDA_PIN  GPIO_PIN_6

//定义AHT20 I2C地址
#define AHT20_I2C_RECEIVE_ADDRESS 0x71
#define AHT20_I2C_SEND_ADDRESS    0x70

//定义软件I2C应答枚举类型
typedef enum
{
    ACK_OK = 0,
    ACK_NO = 1
} I2C_ACK;

//全局变量声明
extern uint8_t AHT20_data[7];

void AHT20_Init(void); //AHT20初始化
void AHT20_Detection_Start(void); //AHT20开始一次检测

#endif