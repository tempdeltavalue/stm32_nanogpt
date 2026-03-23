#include "st7735.h"

extern SPI_HandleTypeDef hspi1;

// 🔥 ЗАДАЄМО ГОРИЗОНТАЛЬНІ РОЗМІРИ (Landscape)
#define TFT_WIDTH  160
#define TFT_HEIGHT 128

// --- БАЗОВІ ФУНКЦІЇ КЕРУВАННЯ ЕКРАНОМ ---

void ST7735_Select() { HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET); }
void ST7735_Unselect() { HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET); }

void ST7735_Reset() {
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(120);
}

void ST7735_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
}

void ST7735_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi1, buff, buff_size, HAL_MAX_DELAY);
}

void ST7735_Init() {
    ST7735_Select();
    ST7735_Reset();

    ST7735_WriteCommand(0x11); // Sleep out
    HAL_Delay(120);

    // 🔥 ПЕРЕВЕРТАЄМО ЕКРАН ДОГОРИ ДРИҐОМ (Landscape Flipped)
    ST7735_WriteCommand(0x36); // MADCTL
    uint8_t d1 = 0xA0;         // 0xA0 перевертає осі X та Y
    ST7735_WriteData(&d1, 1);

    ST7735_WriteCommand(0x3A); // COLMOD
    uint8_t d2 = 0x05;         // 16-bit
    ST7735_WriteData(&d2, 1);

    ST7735_WriteCommand(0x20); // INVON/INVOFF

    ST7735_WriteCommand(0x29); // Display ON
    HAL_Delay(50);
    ST7735_Unselect();
}

void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    ST7735_Select();
    uint8_t data[] = {0x00, x0, 0x00, x1};
    ST7735_WriteCommand(0x2A);
    ST7735_WriteData(data, 4);

    uint8_t data2[] = {0x00, y0, 0x00, y1};
    ST7735_WriteCommand(0x2B);
    ST7735_WriteData(data2, 4);

    ST7735_WriteCommand(0x2C);
    ST7735_Unselect();
}

void ST7735_FillScreen(uint16_t color) {
    uint8_t data[2] = {color >> 8, color & 0xFF};
    ST7735_SetAddressWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    ST7735_Select();
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    for(uint32_t i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
        HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    }
    ST7735_Unselect();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= TFT_WIDTH) || (y >= TFT_HEIGHT)) return;
    ST7735_SetAddressWindow(x, y, x, y);
    uint8_t data[] = {color >> 8, color & 0xFF};
    ST7735_Select();
    ST7735_WriteData(data, 2);
    ST7735_Unselect();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if((x >= TFT_WIDTH) || (y >= TFT_HEIGHT)) return;
    if((x + w - 1) >= TFT_WIDTH) w = TFT_WIDTH - x;
    if((y + h - 1) >= TFT_HEIGHT) h = TFT_HEIGHT - y;

    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7735_Select();
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);

    for(uint16_t j = h; j > 0; j--) {
        for(uint16_t i = w; i > 0; i--) {
            HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
        }
    }
    ST7735_Unselect();
}

// --- НОВІ ФУНКЦІЇ ДЛЯ GFX ШРИФТІВ ---
void ST7735_DrawCharGFX(uint16_t x, uint16_t y, uint8_t c, const GFXfont *gfxFont, uint16_t color, uint16_t bgcolor) {
    if(c < gfxFont->first || c > gfxFont->last) return;

    c -= gfxFont->first;
    GFXglyph *glyph  = (GFXglyph*)&gfxFont->glyph[c];
    uint8_t  *bitmap = (uint8_t*)gfxFont->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width, h = glyph->height;
    int8_t   xo = glyph->xOffset, yo = glyph->yOffset;
    uint8_t  xAdvance = glyph->xAdvance;
    uint8_t  yAdvance = gfxFont->yAdvance;
    uint8_t  xx, yy, bits = 0, bit = 0;

    // Налаштування відступів
    int8_t letter_spacing = -4; // Зближуємо літери горизонтально
    int8_t line_spacing = 8;    // Збільшуємо відстань між рядками, щоб не було накладання

    // Базова лінія для шрифту 12-го розміру
    int16_t baseline = y + 10;

    int8_t effective_advance = xAdvance + letter_spacing;
    if (effective_advance < 1) effective_advance = 1;

    // Очищення фону під символом
    if (color != bgcolor) {
        ST7735_FillRectangle(x, y, effective_advance, yAdvance + line_spacing, bgcolor);
    }

    // Малювання символу
    for(yy=0; yy<h; yy++) {
        for(xx=0; xx<w; xx++) {
            if(!(bit++ & 7)) {
                bits = bitmap[bo++];
            }
            if(bits & 0x80) {
                ST7735_DrawPixel(x + xo + xx, baseline + yo + yy, color);
            }
            bits <<= 1;
        }
    }
}

void ST7735_WriteStringGFX(uint16_t *cursor_x, uint16_t *cursor_y, const char* str, const GFXfont *font, uint16_t color, uint16_t bgcolor) {
    // Ці значення мають бути ідентичними до значень у DrawCharGFX
    int8_t letter_spacing = -3;
    int8_t line_spacing = 8;

    while (*str) {
        uint32_t unicode = 0;
        uint8_t b = (uint8_t)*str;

        if (b < 128) { unicode = b; str++; }
        else if ((b & 0xE0) == 0xC0) {
            unicode = ((b & 0x1F) << 6) | ((uint8_t)str[1] & 0x3F);
            str += 2;
        } else if (b >= 192) {
            unicode = b - 192 + 0x0410;
            str++;
        } else { str++; continue; }

        uint8_t idx = 0;
        if (unicode < 128) idx = unicode;
        else if (unicode >= 0x0410 && unicode <= 0x042F) idx = unicode - 0x0410 + 0x90;
        else if (unicode >= 0x0430 && unicode <= 0x043F) idx = unicode - 0x0430 + 0xB0;
        else if (unicode >= 0x0440 && unicode <= 0x044F) idx = unicode - 0x0440 + 0x80;
        else if (unicode == 0x0406) idx = 0xC4;
        else if (unicode == 0x0456) idx = 0xC5;
        else if (unicode == 0x0407) idx = 0xC6;
        else if (unicode == 0x0457) idx = 0xC7;
        else if (unicode == 0x0404) idx = 0xC2;
        else if (unicode == 0x0454) idx = 0xC3;
        else if (unicode == 0x0490) idx = 0xC8;
        else if (unicode == 0x0491) idx = 0xC9;
        else idx = 0x20;

        if (idx < font->first || idx > font->last) idx = 0x20;

        uint8_t char_width = font->glyph[idx - font->first].xAdvance;
        int8_t effective_advance = char_width + letter_spacing;
        if (effective_advance < 1) effective_advance = 1;

        if (*cursor_x + effective_advance > TFT_WIDTH || b == '\n') {
            *cursor_x = 0;
            *cursor_y += font->yAdvance + line_spacing;
        }

        if (*cursor_y + font->yAdvance > TFT_HEIGHT) {
            *cursor_y = 0;
            ST7735_FillScreen(bgcolor);
        }

        if (b != '\n') {
            ST7735_DrawCharGFX(*cursor_x, *cursor_y, idx, font, color, bgcolor);
            *cursor_x += effective_advance;
        }
    }
}
