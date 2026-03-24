#ifndef __ST7735_H__
#define __ST7735_H__

#include "stm32f4xx_hal.h"
#include "fonts.h"

// ПІНИ
#define ST7735_RES_GPIO_Port GPIOB
#define ST7735_RES_Pin       GPIO_PIN_1

#define ST7735_CS_GPIO_Port  GPIOB
#define ST7735_CS_Pin        GPIO_PIN_0

#define ST7735_DC_GPIO_Port  GPIOB
#define ST7735_DC_Pin        GPIO_PIN_2

// КОЛЬОРИ
#define ST7735_BLACK   0x0000
#define ST7735_BLUE    0x001F
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF

// ФУНКЦІЇ
void ST7735_Init(void);
void ST7735_FillScreen(uint16_t color);
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

void ST7735_DrawCharGFX(uint16_t x, uint16_t y, uint8_t c, const GFXfont *gfxFont, uint16_t color, uint16_t bgcolor);
void ST7735_WriteStringGFX(uint16_t *cursor_x, uint16_t *cursor_y, const char* str, const GFXfont *font, uint16_t color, uint16_t bgcolor);

#endif
