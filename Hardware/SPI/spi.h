#ifndef _SPI_H
#define _SPI_H

#include "gd32f4xx.h"
#include "systick.h"

/* 引脚定义   
复用AF5
PF7    SPI4_SCK
PF8    SPI4_MISO  这个没有用到
PF9    SPI4_MOSI

PB1    RESET
PB15	 DC
PB13   CS
PG13	 BLK
*/      

void spi_config(void);
uint8_t spi_ReadWriteByte(uint8_t dat);

#endif
