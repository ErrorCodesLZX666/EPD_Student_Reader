#include "SPI_Init.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"

#include "gd32f4xx.h"
#include "systick.h"
#include <stdio.h>
#include "Usart0.h"

//#define u8 uint8_t


void EPD_GPIOInit(void)
{
	rcu_periph_clock_enable(EPD_BUSY_GPIO_CLK);
	rcu_periph_clock_enable(EPD_SCL_GPIO_CLK);
	rcu_periph_clock_enable(EPD_SDA_GPIO_CLK);
	rcu_periph_clock_enable(EPD_RES_GPIO_CLK);
	rcu_periph_clock_enable(EPD_DC_GPIO_CLK);
	rcu_periph_clock_enable(EPD_CS_GPIO_CLK);
	/*rcu_periph_clock_enable(EPD_SCL_GPIO_CLK|EPD_SDA_GPIO_CLK|EPD_RES_GPIO_CLK
												 |EPD_DC_GPIO_CLK|EPD_CS_GPIO_CLK|EPD_BUSY_GPIO_CLK);*/
	
	
	gpio_mode_set(EPD_SCL_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,EPD_SCL_GPIO_PIN);
	gpio_output_options_set(EPD_SCL_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_MAX,EPD_SCL_GPIO_PIN);
	
//	GPIO_InitTypeDef GPIO_InitStructure;
//	RCC_AHB1PeriphClockCmd(EPD_SCL_GPIO_CLK|EPD_SDA_GPIO_CLK|EPD_RES_GPIO_CLK
//												 |EPD_DC_GPIO_CLK|EPD_CS_GPIO_CLK|EPD_BUSY_GPIO_CLK,ENABLE);

	
	gpio_mode_set(EPD_SCL_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,EPD_SCL_GPIO_PIN);
	gpio_output_options_set(EPD_SCL_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_MAX,EPD_SCL_GPIO_PIN);
	
//	GPIO_InitStructure.GPIO_Pin=EPD_SCL_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
//	GPIO_Init(EPD_SCL_GPIO_PORT,&GPIO_InitStructure);
//	

	gpio_mode_set(EPD_SDA_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,EPD_SDA_GPIO_PIN);
	gpio_output_options_set(EPD_SDA_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_MAX,EPD_SDA_GPIO_PIN);
//	GPIO_InitStructure.GPIO_Pin=EPD_SDA_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
//	GPIO_Init(EPD_SDA_GPIO_PORT,&GPIO_InitStructure);
//	

	gpio_mode_set(EPD_RES_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,EPD_RES_GPIO_PIN);
	gpio_output_options_set(EPD_RES_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_MAX,EPD_RES_GPIO_PIN);
//	GPIO_InitStructure.GPIO_Pin=EPD_RES_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
//	GPIO_Init(EPD_RES_GPIO_PORT,&GPIO_InitStructure);
//	
	
	gpio_mode_set(EPD_DC_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,EPD_DC_GPIO_PIN);
	gpio_output_options_set(EPD_DC_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_MAX,EPD_DC_GPIO_PIN);
//	GPIO_InitStructure.GPIO_Pin=EPD_DC_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
//	GPIO_Init(EPD_DC_GPIO_PORT,&GPIO_InitStructure);
//	

	gpio_mode_set(EPD_CS_GPIO_PORT,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,EPD_CS_GPIO_PIN);
	gpio_output_options_set(EPD_CS_GPIO_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_MAX,EPD_CS_GPIO_PIN);
//	GPIO_InitStructure.GPIO_Pin=EPD_CS_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Mode =GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
//	GPIO_Init(EPD_CS_GPIO_PORT,&GPIO_InitStructure);
//	
	
	gpio_mode_set(EPD_BUSY_GPIO_PORT,GPIO_MODE_INPUT,GPIO_PUPD_PULLUP,EPD_BUSY_GPIO_PIN);


}


void EPD_WR_Bus(u8 dat)
{
	u8 i;
	EPD_CS_Clr();
	for(i=0;i<8;i++)
	{
		EPD_SCL_Clr();
		if(dat&0x80)
		{
			EPD_SDA_Set();
		}
		else
		{
			EPD_SDA_Clr();
		}
		EPD_SCL_Set();
		dat<<=1;
	}
	EPD_CS_Set();	
}

void EPD_WR_REG(u8 reg)
{
	EPD_DC_Clr();
	EPD_WR_Bus(reg);
	EPD_DC_Set();
}
void EPD_WR_DATA8(u8 dat)
{
	EPD_DC_Set();
	EPD_WR_Bus(dat);
	EPD_DC_Set();
}





