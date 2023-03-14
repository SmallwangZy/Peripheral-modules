#ifndef __OLED_H_
#define __OLED_H_

#include "i2c.h"
#include "gpio.h"
#include "main.h"

#define OLED_ADD 0x78 //…Ë±∏µÿ÷∑
#define Defalut_Char_Size  16
#define Defalut_Str_Size  2

void oled_cmd(uint8_t i2c_cmd);
void oled_data(uint8_t i2c_data);
void oled_setpoint(uint8_t x,uint8_t y);
void oled_full(uint8_t data);
void oled_clear();
void oled_init(void);
void oled_show_char(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size);
void oled_show_string(uint8_t x, uint8_t y, char ch[], uint8_t TextSize);
uint32_t oled_pow(uint8_t m, uint8_t n);
void oled_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);
void ShowPage();
void oled_clear_area(uint8_t x0,uint8_t y0,uint8_t x1,uint8_t x2);


#endif 