#include "gd32vf103_gpio.h"
#include "extram.h"
#include "spi0.h"

#define SELECT()         gpio_bit_reset(GPIOA, GPIO_PIN_3)
#define DESELECT()       gpio_bit_set(GPIOA, GPIO_PIN_3)

#define COMMAND_READ     0x03
#define COMMAND_WRITE    0x02

void extram_init(void)
{
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
	DESELECT();
}

void *extram_read(u32 addr, void *data, u32 size)
{
	u32 i;
	u8 *data8;
	data8 = (u8 *)data;
	SELECT();
	spi0_rxtx(COMMAND_READ);
	spi0_rxtx((u8)((addr >> 16) & 0xFF));
	spi0_rxtx((u8)((addr >> 8) & 0xFF));
	spi0_rxtx((u8)(addr & 0xFF);
	for(i = 0; i < size; ++i)
	{
		data8[i] = spi0_rxtx(0xFF);
	}

	DESELECT();
	return data;
}

void extram_write(u32 addr, void *data, u32 size)
{
	u32 i;
	u8 *data8;
	data8 = (u8 *)data;
	SELECT();
	spi0_rxtx(COMMAND_WRITE);
	spi0_rxtx((u8)((addr >> 16) & 0xFF));
	spi0_rxtx((u8)((addr >> 8) & 0xFF));
	spi0_rxtx((u8)(addr & 0xFF));
	for(i = 0; i < size; ++i)
	{
		spi0_rxtx(data8[i]);
	}

	DESELECT();
}

