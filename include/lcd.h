#ifndef __LCD_H__
#define __LCD_H__

#include "types.h"

#define LCD_ORIENTATION 2

#define LCD_WHITE        0xFFFF
#define LCD_BLACK        0x0000
#define LCD_BLUE         0x001F
#define LCD_RED          0xF800
#define LCD_MAGENTA      0xF81F
#define LCD_GREEN        0x07E0
#define LCD_CYAN         0x7FFF
#define LCD_YELLOW       0xFFE0

#if LCD_ORIENTATION == 0 || LCD_ORIENTATION == 1
#define LCD_W          80
#define LCD_H         160
#else
#define LCD_W         160
#define LCD_H          80
#endif

#define lcd_clear(color) lcd_rect(0, 0, LCD_W, LCD_H, color)
void lcd_init(void);
void lcd_address_set(u16 x0, u16 y0, u16 x1, u16 y1);
void lcd_write_data_16(u16 data);
void lcd_pixel(i16 x, i16 y, u16 color);
void lcd_rect(u16 x, u16 y, u16 w, u16 h, u16 color);
void lcd_line(i16 x0, i16 y0, i16 x1, i16 y1, u16 color);
void lcd_draw_circle(i16 x0, i16 y0, i16 r, u16 color);
void lcd_string(u16 x, u16 y, u16 fg, u16 bg, char *s);

#endif

