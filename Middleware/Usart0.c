#include "Usart0.h"
// 中间件初始化函数
void usart0_init(){

	uint32_t usartx_tx_rcu = RCU_GPIOA;
    uint32_t usartx_tx_port = GPIOA;
    uint32_t usartx_tx_pin = GPIO_PIN_9;
    uint32_t usartx_tx_af = GPIO_AF_7;

    uint32_t usartx_rx_rcu = RCU_GPIOA;
    uint32_t usartx_rx_port = GPIOA;
    uint32_t usartx_rx_pin = GPIO_PIN_10;
    uint32_t usartx_rx_af = GPIO_AF_7;

    uint32_t usartx = USART0;
    uint32_t usartx_rcu = RCU_USART0;
    uint32_t usartx_irqn = USART0_IRQn;

    uint32_t usartx_p_baudrate = 115200;
    uint32_t usartx_p_parity = USART_PM_NONE;
    uint32_t usartx_p_wl = USART_WL_8BIT;
    uint32_t usartx_p_stop_bit = USART_STB_1BIT;
    uint32_t usartx_p_data_first = USART_MSBF_LSB;

    /************** gpio config **************/
    // tx
    rcu_periph_clock_enable(usartx_tx_rcu);	// 配置时钟
    gpio_mode_set(usartx_tx_port, GPIO_MODE_AF, GPIO_PUPD_NONE, usartx_tx_pin);
    gpio_af_set(usartx_tx_port, usartx_tx_af, usartx_tx_pin);
    gpio_output_options_set(usartx_tx_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, usartx_tx_pin);
    // rx
    rcu_periph_clock_enable(usartx_rx_rcu); // 配置时钟
    gpio_mode_set(usartx_rx_port, GPIO_MODE_AF, GPIO_PUPD_NONE, usartx_rx_pin);
    gpio_af_set(usartx_rx_port, usartx_rx_af, usartx_rx_pin);
    gpio_output_options_set(usartx_rx_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, usartx_rx_pin);

    /************** usart config **************/
    // 串口时钟
    rcu_periph_clock_enable(RCU_USART0);
    // USART复位
    usart_deinit(usartx);

    usart_baudrate_set(usartx, usartx_p_baudrate);	// 波特率
    usart_parity_config(usartx, usartx_p_parity); // 校验位
    usart_word_length_set(usartx, usartx_p_wl); // 数据位数
    usart_stop_bit_set(usartx, usartx_p_stop_bit); // 停止位
    usart_data_first_config(usartx, usartx_p_data_first); // 先发送高位还是低位

    // 发送功能配置
    usart_transmit_config(usartx, USART_TRANSMIT_ENABLE); 

    // 接收功能配置
    usart_receive_config(usartx, USART_RECEIVE_ENABLE);
    // 接收中断配置
    nvic_irq_enable(usartx_irqn, 2, 2);
    // usart int rbne
    usart_interrupt_enable(usartx, USART_INT_RBNE);
    usart_interrupt_enable(usartx, USART_INT_IDLE);

    // 使能串口
    usart_enable(usartx); 
}


void usart0_send_byte(uint8_t data){
    //通过USART发送
    usart_data_transmit(USART0, data);
    //判断缓冲区是否已经空了
    //FlagStatus state = usart_flag_get(USART_NUM,USART_FLAG_TBE);
	//delay_1ms(10);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
}


void usart0_send_data(uint8_t *data, uint8_t len){
	
	//满足: 1.data指针不为空  2.发送的数据不是\0结束标记
	// 		3. 并且判断len是否等于0如果等于0就代表所有的数据已经发送完毕
	while(data && *data && len){
		usart0_send_byte((uint8_t)(*data));
		data++;
		len --;			// 每发送一个数据就将len--。直到等于0
	}
}

void usart0_send_string(uint8_t *str) {
	while(str && *str){
		usart0_send_byte((uint8_t)(*str));
		str++;
	}
}


// 串口打印功能实现
int fputc(int ch, FILE *f) {
    usart0_send_byte((uint8_t)ch);
	// vTaskDelay();
    return ch;
}



#define USART_RECEIVE_LENGTH  1024
//串口接收缓冲区大小
static uint8_t g_recv_buff[USART_RECEIVE_LENGTH];   // 接收缓冲区
//接收到字符存放的位置
static int g_recv_length = 0;

// 定义中断中要使用调用的函数
void USART0_IRQHandler(void) {
    if ((usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)) == SET) {
		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
        uint16_t value = usart_data_receive(USART0);
        g_recv_buff[g_recv_length] = value;		
        g_recv_length++;
    }
    if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE) == SET) {
        //读取缓冲区,清空缓冲区
        usart_data_receive(USART0);
        g_recv_buff[g_recv_length] = '\0';

        // TODO: g_recv_buff为接收的数据，g_recv_length为接收的长度
        //printf("%s", g_recv_buff);
		// 这里就调用回调函数
		usart0_on_recive(g_recv_buff,g_recv_length);

        g_recv_length = 0;
    }
}
