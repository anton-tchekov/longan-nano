#include "gd32vf103.h"
#include "usart.h"

#define SIZE (1 << 10)
#define MASK (SIZE - 1)

typedef struct
{
	u32 In;
	u32 Out;
	u8 *Data;
} BUFFER;

BUFFER _tx_buf;
BUFFER _rx_buf;

u8 _rx_data[SIZE];
u8 _tx_data[SIZE];

static void buffer_init(BUFFER *buf, u8 *data)
{
	buf->In = 0;
	buf->Out = 0;
	buf->Data = data;
}

static u8 buffer_out(BUFFER *buf, u8 *b)
{
	if(buf->In == buf->Out)
	{
		return -1;
	}

	*b = buf->Data[buf->Out++];
	buf->Out &= MASK;
	return 0;
}

static u8 buffer_in(BUFFER *buf, u8 b)
{
	if(((buf->In + 1) & MASK) == buf->Out)
	{
		return -1;
	}

	buf->Data[buf->In++] = b;
	buf->In &= MASK;
	return 0;
}

void usart_init(u32 baud)
{
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_USART0);
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
	usart_deinit(USART0);
	usart_baudrate_set(USART0, baud);
	usart_word_length_set(USART0, USART_WL_8BIT);
	usart_stop_bit_set(USART0, USART_STB_1BIT);
	usart_parity_config(USART0, USART_PM_NONE);
	usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
	usart_receive_config(USART0, USART_RECEIVE_ENABLE);
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
	usart_enable(USART0);
	eclic_global_interrupt_enable();
	eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO1);
	eclic_irq_enable(USART0_IRQn, 1, 0);
	usart_interrupt_enable(USART0, USART_INT_TBE);
	usart_interrupt_enable(USART0, USART_INT_RBNE);

	buffer_init(&_rx_buf, _rx_data);
	buffer_init(&_tx_buf, _tx_data);
}

i8 usart_tx(u8 b)
{
	u8 res;
	usart_interrupt_disable(USART0, USART_INT_TBE);
	res = buffer_in(&_tx_buf, b);
	usart_interrupt_enable(USART0, USART_INT_TBE);
	return res;
}

i16 usart_rx(void)
{
	u8 b;
	usart_interrupt_disable(USART0, USART_INT_RBNE);
	if(buffer_out(&_rx_buf, &b))
	{
		return -1;
	}

	usart_interrupt_enable(USART0, USART_INT_RBNE);
	return b;
}

void USART0_IRQHandler(void)
{
	/* rx */
	if(usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) != RESET)
	{
		usart_interrupt_disable(USART0, USART_INT_RBNE);
		buffer_in(&_rx_buf, usart_data_receive(USART0));
		usart_interrupt_enable(USART0, USART_INT_RBNE);
	}

	/* tx */
	if(usart_interrupt_flag_get(USART0, USART_INT_FLAG_TBE) != RESET)
	{
		u8 b;
		usart_interrupt_disable(USART0, USART_INT_TBE);
		if(!buffer_out(&_tx_buf, &b))
		{
			usart_data_transmit(USART0, b);
			usart_interrupt_enable(USART0, USART_INT_TBE);
		}
	}
}

