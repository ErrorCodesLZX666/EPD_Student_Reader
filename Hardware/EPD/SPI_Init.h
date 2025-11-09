#ifndef _SPI_INIT_H_
#define _SPI_INIT_H_

#include "gd32f4xx_rcu.h"
#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"

#define u8 uint8_t

//#define EPD_SCL_GPIO_PORT		GPIOG
//#define EPD_SCL_GPIO_PIN		GPIO_Pin_12	
//#define EPD_SCL_GPIO_CLK		RCC_AHB1Periph_GPIOG

//#define EPD_SDA_GPIO_PORT		GPIOD
//#define EPD_SDA_GPIO_PIN		GPIO_Pin_5	
//#define EPD_SDA_GPIO_CLK		RCC_AHB1Periph_GPIOD

//#define EPD_RES_GPIO_PORT		GPIOD
//#define EPD_RES_GPIO_PIN		GPIO_Pin_4	
//#define EPD_RES_GPIO_CLK		RCC_AHB1Periph_GPIOD

//#define EPD_DC_GPIO_PORT		GPIOD
//#define EPD_DC_GPIO_PIN			GPIO_Pin_15	
//#define EPD_DC_GPIO_CLK			RCC_AHB1Periph_GPIOD

//#define EPD_CS_GPIO_PORT		GPIOD
//#define EPD_CS_GPIO_PIN			GPIO_Pin_1	
//#define EPD_CS_GPIO_CLK			RCC_AHB1Periph_GPIOD

//#define EPD_BUSY_GPIO_PORT	GPIOE
//#define EPD_BUSY_GPIO_PIN		GPIO_Pin_8	
//#define EPD_BUSY_GPIO_CLK		RCC_AHB1Periph_GPIOE



//#define EPD_SCL_Clr() GPIO_ResetBits(EPD_SCL_GPIO_PORT,EPD_SCL_GPIO_PIN)
//#define EPD_SCL_Set() GPIO_SetBits(EPD_SCL_GPIO_PORT,EPD_SCL_GPIO_PIN)

//#define EPD_SDA_Clr() GPIO_ResetBits(EPD_SDA_GPIO_PORT,EPD_SDA_GPIO_PIN)
//#define EPD_SDA_Set() GPIO_SetBits(EPD_SDA_GPIO_PORT,EPD_SDA_GPIO_PIN)

//#define EPD_RES_Clr() GPIO_ResetBits(EPD_RES_GPIO_PORT,EPD_RES_GPIO_PIN)
//#define EPD_RES_Set() GPIO_SetBits(EPD_RES_GPIO_PORT,EPD_RES_GPIO_PIN)

//#define EPD_DC_Clr() GPIO_ResetBits(EPD_DC_GPIO_PORT,EPD_DC_GPIO_PIN)
//#define EPD_DC_Set() GPIO_SetBits(EPD_DC_GPIO_PORT,EPD_DC_GPIO_PIN)

//#define EPD_CS_Clr() GPIO_ResetBits(EPD_CS_GPIO_PORT,EPD_CS_GPIO_PIN)
//#define EPD_CS_Set() GPIO_SetBits(EPD_CS_GPIO_PORT,EPD_CS_GPIO_PIN)

//#define EPD_ReadBusy GPIO_ReadInputDataBit(EPD_BUSY_GPIO_PORT,EPD_BUSY_GPIO_PIN)











#define EPD_SCL_GPIO_PORT		GPIOG
#define EPD_SCL_GPIO_PIN		GPIO_PIN_12	
#define EPD_SCL_GPIO_CLK		RCU_GPIOG

#define EPD_SDA_GPIO_PORT		GPIOD
#define EPD_SDA_GPIO_PIN		GPIO_PIN_5	
#define EPD_SDA_GPIO_CLK		RCU_GPIOD

#define EPD_RES_GPIO_PORT		GPIOD
#define EPD_RES_GPIO_PIN		GPIO_PIN_4	
#define EPD_RES_GPIO_CLK		RCU_GPIOD

#define EPD_DC_GPIO_PORT		GPIOC
#define EPD_DC_GPIO_PIN			GPIO_PIN_7
#define EPD_DC_GPIO_CLK			RCU_GPIOC

#define EPD_CS_GPIO_PORT		GPIOD
#define EPD_CS_GPIO_PIN			GPIO_PIN_1	
#define EPD_CS_GPIO_CLK			RCU_GPIOD

#define EPD_BUSY_GPIO_PORT		GPIOC
#define EPD_BUSY_GPIO_PIN		GPIO_PIN_6	
#define EPD_BUSY_GPIO_CLK		RCU_GPIOC



#define EPD_SCL_Clr() gpio_bit_reset(EPD_SCL_GPIO_PORT,EPD_SCL_GPIO_PIN)
#define EPD_SCL_Set() gpio_bit_set(EPD_SCL_GPIO_PORT,EPD_SCL_GPIO_PIN)

#define EPD_SDA_Clr() gpio_bit_reset(EPD_SDA_GPIO_PORT,EPD_SDA_GPIO_PIN)
#define EPD_SDA_Set() gpio_bit_set(EPD_SDA_GPIO_PORT,EPD_SDA_GPIO_PIN)

#define EPD_RES_Clr() gpio_bit_reset(EPD_RES_GPIO_PORT,EPD_RES_GPIO_PIN)
#define EPD_RES_Set() gpio_bit_set(EPD_RES_GPIO_PORT,EPD_RES_GPIO_PIN)

#define EPD_DC_Clr() gpio_bit_reset(EPD_DC_GPIO_PORT,EPD_DC_GPIO_PIN)
#define EPD_DC_Set() gpio_bit_set(EPD_DC_GPIO_PORT,EPD_DC_GPIO_PIN)

#define EPD_CS_Clr() gpio_bit_reset(EPD_CS_GPIO_PORT,EPD_CS_GPIO_PIN)
#define EPD_CS_Set() gpio_bit_set(EPD_CS_GPIO_PORT,EPD_CS_GPIO_PIN)

#define EPD_ReadBusy gpio_input_bit_get(EPD_BUSY_GPIO_PORT,EPD_BUSY_GPIO_PIN)

// 兼容数据类型
//#define u8 uint8_t
//#define u16 uint16_t
//#define u32 uint32_t


void EPD_GPIOInit(void);  //初始化EPD对应GPIO口
void EPD_WR_Bus(u8 dat);	//写入一个字节
void EPD_WR_REG(u8 reg);	//写入指令
void EPD_WR_DATA8(u8 dat);//写入数据

#endif



