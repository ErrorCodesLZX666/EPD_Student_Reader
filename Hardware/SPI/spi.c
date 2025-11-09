#include "spi.h"


/*!
    \brief      configure different peripheral clocks
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void rcu_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOF);  // 使用F端口
    rcu_periph_clock_enable(RCU_SPI4);  // 使能SPI4
	
	    /* configure SPI4 GPIO */
    gpio_af_set(GPIOF, GPIO_AF_5, GPIO_PIN_7 | GPIO_PIN_8| GPIO_PIN_9);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7 |GPIO_PIN_8 | GPIO_PIN_9);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
	
		gpio_bit_set(GPIOF,GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
}

/*!
    \brief      configure the SPI peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void spi_config(void)
{
	  spi_parameter_struct spi_init_struct;
		rcu_config();
    /* configure SPI4 parameter */
		spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;  // 传输模式全双工 SPI_TRANSMODE_FULLDUPLEX
    spi_init_struct.device_mode          = SPI_MASTER;   // 配置为主机
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT; // 8位数据
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;//SPI_CK_PL_HIGH_PH_2EDGE
    spi_init_struct.nss                  = SPI_NSS_SOFT;  // 软件cs
   // spi_init_struct.prescale             = SPI_PSC_32;
	  spi_init_struct.prescale             = SPI_PSC_2;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI4, &spi_init_struct);
	
	  /* enable SPI */
    spi_enable(SPI4);

}

uint8_t spi_ReadWriteByte(uint8_t dat)
{
	 while(RESET == spi_i2s_flag_get(SPI4, SPI_FLAG_TBE));
        spi_i2s_data_transmit(SPI4, dat);
	
	 while(RESET == spi_i2s_flag_get(SPI4, SPI_FLAG_RBNE));
        return spi_i2s_data_receive(SPI4);
}

