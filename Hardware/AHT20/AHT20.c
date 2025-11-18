/********************************************************************************
  * 测试硬件：立创·梁山派开发板GD32F470ZGT6    使用主频200Mhz    晶振25Mhz
  * 版 本 号: V1.0
  * 修改作者: LCKFB
  * 修改日期: 2022年04月19日
  * 功能介绍:
  ******************************************************************************
  * 梁山派软硬件资料与相关扩展板软硬件资料官网全部开源
  * 开发板官网：www.lckfb.com
  * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
  * 立创论坛：club.szlcsc.com
  * 其余模块移植手册：https://dri8c0qdfb.feishu.cn/docx/EGRVdxunnohkrNxItYTcrwAnnHe
  * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
  * 不靠卖板赚钱，以培养中国工程师为己任
*********************************************************************************/

#include "AHT20.h"
#include "gd32f4xx.h"

uint8_t AHT20_data[7] = {0};

/******************************************************************
 * 函 数 名 称：AHT20_I2C_IO_Init
 * 函 数 说 明：AHT20软件I2C通讯IO口初始化
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
******************************************************************/
static void AHT20_I2C_IO_Init(void)
{
    rcu_periph_clock_enable(AHT20_SCL_RCU);
    rcu_periph_clock_enable(AHT20_SDA_RCU);

    gpio_mode_set(AHT20_SCL_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, AHT20_SCL_PIN);
    gpio_output_options_set(AHT20_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_MAX, AHT20_SCL_PIN);
    gpio_mode_set(AHT20_SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, AHT20_SDA_PIN);
    gpio_output_options_set(AHT20_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_MAX, AHT20_SDA_PIN);
}

/******************************************************************
 * 函 数 名 称：AHT20_I2C_Start
 * 函 数 说 明：软件I2C产生开始信号
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
******************************************************************/
static void AHT20_I2C_Start(void)
{
    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, SET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, SET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, RESET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, RESET);
    AHT20_I2C_Delay;
}

/******************************************************************
 * 函 数 名 称：AHT20_I2C_Stop
 * 函 数 说 明：软件I2C产生结束信号
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
******************************************************************/
static void AHT20_I2C_Stop(void)
{
    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, RESET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, SET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, SET);
    AHT20_I2C_Delay;
}

/******************************************************************
 * 函 数 名 称：AHT20_I2C_Send_Ack
 * 函 数 说 明：软件I2C产生应答信号
 * 函 数 形 参：Ack=发送的应答
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
******************************************************************/
static void AHT20_I2C_Send_Ack(I2C_ACK Ack)
{
    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, (FlagStatus)Ack);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, SET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, RESET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, SET);
    AHT20_I2C_Delay;
}

/******************************************************************
 * 函 数 名 称：AHT20_I2C_Receive_Ack
 * 函 数 说 明：软件I2C接收应答信号
 * 函 数 形 参：无
 * 函 数 返 回：ACK_OK=有应答，ACK_NO=无应答
 * 作       者：LC
 * 备       注：无
******************************************************************/
static I2C_ACK AHT20_I2C_Receive_Ack(void)
{
    I2C_ACK Ack;

    gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, SET);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, SET);
    AHT20_I2C_Delay;
    Ack = (I2C_ACK)gpio_input_bit_get(AHT20_SDA_PORT, AHT20_SDA_PIN);
    AHT20_I2C_Delay;
    gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, RESET);
    AHT20_I2C_Delay;

    return Ack;
}

/******************************************************************
 * 函 数 名 称：AHT20_I2C_Send_Byte
 * 函 数 说 明：软件I2C发送一个字节
 * 函 数 形 参：Byte=发送的字节
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
******************************************************************/
static void AHT20_I2C_Send_Byte(uint8_t Byte)
{
    uint8_t i;
    I2C_ACK Ack = ACK_NO;

        for (i = 0; i < 8; i ++)
        {
        gpio_bit_write(AHT20_SDA_PORT, AHT20_SDA_PIN, (FlagStatus)(Byte & (0x80 >> i)));
        AHT20_I2C_Delay;
        gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, SET);
        AHT20_I2C_Delay;
        gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, RESET);
        AHT20_I2C_Delay;
        }
    Ack = AHT20_I2C_Receive_Ack();

         if (Ack == ACK_OK)
    {

    }
    else
    {

    }
}

/******************************************************************
 * 函 数 名 称：AHT20_I2C_Receive_Byte
 * 函 数 说 明：软件I2C接受的字节
 * 函 数 形 参：Ack=接受完字节后给出的应答信号
 * 函 数 返 回：接收到的字节
 * 作       者：LC
 * 备       注：无
******************************************************************/
static uint8_t AHT20_I2C_Receive_Byte(I2C_ACK Ack)
{
    uint8_t Byte;
    uint8_t i;

    for (i = 8; i > 0; i--)
    {
        gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, SET);
        AHT20_I2C_Delay;
        Byte |= (gpio_input_bit_get(AHT20_SDA_PORT, AHT20_SDA_PIN) << (i - 1));
        gpio_bit_write(AHT20_SCL_PORT, AHT20_SCL_PIN, RESET);
        AHT20_I2C_Delay;
    }

    AHT20_I2C_Send_Ack(Ack);

    return Byte;
}

/******************************************************************
 * 函 数 名 称：AHT20_Init
 * 函 数 说 明：AHT20初始化
 * 函 数 形 参：无
 * 函 数 返 回：接收到的字节
 * 作       者：LC
 * 备       注：无
******************************************************************/
void AHT20_Init(void)
{
        AHT20_I2C_IO_Init();
        delay_1ms(100);
}

/******************************************************************
 * 函 数 名 称：AHT20_Detection_Start
 * 函 数 说 明：AHT20开始一次测量
 * 函 数 形 参：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
******************************************************************/
void AHT20_Detection_Start(void)
{
    AHT20_I2C_Start();
    AHT20_I2C_Send_Byte(AHT20_I2C_SEND_ADDRESS);
    AHT20_I2C_Send_Byte(0xAC);
    AHT20_I2C_Send_Byte(0x33);
    AHT20_I2C_Send_Byte(0x00);
    AHT20_I2C_Stop();

    delay_1ms(80);

    AHT20_I2C_Start();
    AHT20_I2C_Send_Byte(AHT20_I2C_RECEIVE_ADDRESS);
    AHT20_data[0] = AHT20_I2C_Receive_Byte(ACK_OK);
    AHT20_data[1] = AHT20_I2C_Receive_Byte(ACK_OK);
    AHT20_data[2] = AHT20_I2C_Receive_Byte(ACK_OK);
    AHT20_data[3] = AHT20_I2C_Receive_Byte(ACK_OK);
    AHT20_data[4] = AHT20_I2C_Receive_Byte(ACK_OK);
    AHT20_data[5] = AHT20_I2C_Receive_Byte(ACK_OK);
    AHT20_data[6] = AHT20_I2C_Receive_Byte(ACK_NO);
    AHT20_I2C_Stop();
}