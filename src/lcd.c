#include "lcd.h"
#include "font.h"
#include "systick.h"
#include "gd32vf103_gpio.h"
#include "spi0.h"
#include <stdlib.h>

#define LCD_CMD         0
#define LCD_DATA        1

/* CS/PB2 */
#define lcd_cs_clear()   gpio_bit_reset(GPIOB, GPIO_PIN_2)
#define lcd_cs_set()     gpio_bit_set(GPIOB, GPIO_PIN_2)

/* RES/PB1 */
#define lcd_rst_clear()  gpio_bit_reset(GPIOB, GPIO_PIN_1)     
#define lcd_rst_set()    gpio_bit_set(GPIOB, GPIO_PIN_1)

/* DC/PB0 */
#define lcd_dc_clear()   gpio_bit_reset(GPIOB, GPIO_PIN_0)      
#define lcd_dc_set()     gpio_bit_set(GPIOB, GPIO_PIN_0)

static void lcd_write_bus(u8 data)
{
	lcd_cs_clear();
	spi0_rxtx(data);
	lcd_cs_set();
}

static void lcd_write_data_8(u8 data)
{
	lcd_dc_set();
	lcd_write_bus(data);
}

void lcd_write_data_16(u16 data)
{
	lcd_dc_set();
	lcd_write_bus(data >> 8);
	lcd_write_bus(data);
}

static void lcd_write_register(u8 data)
{
	lcd_dc_clear();
	lcd_write_bus(data);
}

void lcd_address_set(u16 x0, u16 y0, u16 x1, u16 y1)
{
#if LCD_ORIENTATION == 0 || LCD_ORIENTATION == 1
	lcd_write_register(0x2a);
	lcd_write_data_16(x0 + 26);
	lcd_write_data_16(x1 + 26);
	lcd_write_register(0x2b);
	lcd_write_data_16(y0 + 1);
	lcd_write_data_16(y1 + 1);
	lcd_write_register(0x2c);
#else
	lcd_write_register(0x2a);
	lcd_write_data_16(x0 + 1);
	lcd_write_data_16(x1 + 1);
	lcd_write_register(0x2b);
	lcd_write_data_16(y0 + 26);
	lcd_write_data_16(y1 + 26);
	lcd_write_register(0x2c);
#endif
}

void lcd_init(void)
{
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
	lcd_cs_set();
	/* rst_clear(), dc_clear() */
	gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1);
	delay_ms(200);
	lcd_rst_set();
	delay_ms(20);
	lcd_write_register(0x11);
	delay_ms(100);
	lcd_write_register(0x21);
	lcd_write_register(0xB1);
	lcd_write_data_8(0x05);
	lcd_write_data_8(0x3A);
	lcd_write_data_8(0x3A);
	lcd_write_register(0xB2);
	lcd_write_data_8(0x05);
	lcd_write_data_8(0x3A);
	lcd_write_data_8(0x3A);
	lcd_write_register(0xB3);
	lcd_write_data_8(0x05);
	lcd_write_data_8(0x3A);
	lcd_write_data_8(0x3A);
	lcd_write_data_8(0x05);
	lcd_write_data_8(0x3A);
	lcd_write_data_8(0x3A);
	lcd_write_register(0xB4);
	lcd_write_data_8(0x03);
	lcd_write_register(0xC0);
	lcd_write_data_8(0x62);
	lcd_write_data_8(0x02);
	lcd_write_data_8(0x04);
	lcd_write_register(0xC1);
	lcd_write_data_8(0xC0);
	lcd_write_register(0xC2);
	lcd_write_data_8(0x0D);
	lcd_write_data_8(0x00);
	lcd_write_register(0xC3);
	lcd_write_data_8(0x8D);
	lcd_write_data_8(0x6A);
	lcd_write_register(0xC4);
	lcd_write_data_8(0x8D);
	lcd_write_data_8(0xEE);
	lcd_write_register(0xC5);
	lcd_write_data_8(0x0E);
	lcd_write_register(0xE0);
	lcd_write_data_8(0x10);
	lcd_write_data_8(0x0E);
	lcd_write_data_8(0x02);
	lcd_write_data_8(0x03);
	lcd_write_data_8(0x0E);
	lcd_write_data_8(0x07);
	lcd_write_data_8(0x02);
	lcd_write_data_8(0x07);
	lcd_write_data_8(0x0A);
	lcd_write_data_8(0x12);
	lcd_write_data_8(0x27);
	lcd_write_data_8(0x37);
	lcd_write_data_8(0x00);
	lcd_write_data_8(0x0D);
	lcd_write_data_8(0x0E);
	lcd_write_data_8(0x10);
	lcd_write_register(0xE1);
	lcd_write_data_8(0x10);
	lcd_write_data_8(0x0E);
	lcd_write_data_8(0x03);
	lcd_write_data_8(0x03);
	lcd_write_data_8(0x0F);
	lcd_write_data_8(0x06);
	lcd_write_data_8(0x02);
	lcd_write_data_8(0x08);
	lcd_write_data_8(0x0A);
	lcd_write_data_8(0x13);
	lcd_write_data_8(0x26);
	lcd_write_data_8(0x36);
	lcd_write_data_8(0x00);
	lcd_write_data_8(0x0D);
	lcd_write_data_8(0x0E);
	lcd_write_data_8(0x10);
	lcd_write_register(0x3A);
	lcd_write_data_8(0x05);
	lcd_write_register(0x36);
#if LCD_ORIENTATION == 0
	lcd_write_data_8(0x08);
#elif LCD_ORIENTATION == 1
	lcd_write_data_8(0xC8);
#elif LCD_ORIENTATION == 2
	lcd_write_data_8(0x78);
#else
	lcd_write_data_8(0xA8);
#endif
	lcd_write_register(0x29);
}

void lcd_pixel(i16 x, i16 y, u16 color)
{
	if(x >= 0 && y >= 0 && x < LCD_W && y < LCD_H)
	{
		lcd_address_set(x, y, x, y);
		lcd_write_data_16(color);
	}
}

void lcd_rect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
	u32 n;
	if(x + w > LCD_W || y + h > LCD_H)
	{
		return;
	}

	lcd_address_set(x, y, x + w - 1, y + h - 1);
	n = w * h;
	while(n--)
	{
		lcd_write_data_16(color);
	}
}

void lcd_line(i16 x0, i16 y0, i16 x1, i16 y1, u16 color)
{
	i16 dx, dy, sx, sy, e0, e1;
	dx = abs(x1 - x0);
	sx = x0 < x1 ? 1 : -1;
	dy = abs(y1 - y0);
	sy = y0 < y1 ? 1 : -1;
	e0 = (dx > dy ? dx : -dy) / 2;
	while(x0 != x1 && y0 != y1)
	{
		lcd_pixel(x0, y0, color);
		e1 = e0;
		if(e1 > -dx)
		{
			e0 -= dy;
			x0 += sx;
		}

		if(e1 < dy)
		{
			e0 += dx;
			y0 += sy;
		}
	}
}

void lcd_draw_circle(i16 x0, i16 y0, i16 r, u16 color)
{
	i16 x, y, dx, dy, err;
	x = r - 1;
	y = 0;
	dx = 1;
	dy = 1;
	err = dx - (r << 1);
	while(x >= y)
	{
		lcd_pixel(x0 + x, y0 + y, color);
		lcd_pixel(x0 + y, y0 + x, color);
		lcd_pixel(x0 - y, y0 + x, color);
		lcd_pixel(x0 - x, y0 + y, color);
		lcd_pixel(x0 - x, y0 - y, color);
		lcd_pixel(x0 - y, y0 - x, color);
		lcd_pixel(x0 + y, y0 - x, color);
		lcd_pixel(x0 + x, y0 - y, color);
		if(err <= 0)
		{
			++y;
			err += dy;
			dy += 2;
		}

		if(err > 0)
		{
			--x;
			dx += 2;
			err += dx - (r << 1);
		}
	}
}

void lcd_string(u16 x, u16 y, u16 fg, u16 bg, char *s)
{
	u8 width, col, row, data;
	char *p, c;
	for(p = s, width = 0; (c = *p); ++p)
	{
		if(c >= 32 && c <= 126)
		{
			width += FontDefault.Widths[(u8)(c - 32)];
		}
	}

	if(x + width >= LCD_W || y + FontDefault.Height >= LCD_H)
	{
		return;
	}

	lcd_address_set(x, y, x + width - 1, y + FontDefault.Height - 1);
	for(row = 0; row < FontDefault.Height; ++row)
	{
		for(p = s; (c = *p); ++p)
		{
			if(c >= 32 && c <= 126)
			{
				c -= 32;
				for(col = 0; col < FontDefault.Widths[(u8)c]; ++col)
				{
					data = FontDefault.Chars[(u8)c * FontDefault.Height + row];
					lcd_write_data_16(((data >> col) & 1) ? fg : bg);
				}
			}
		}
	}
}

