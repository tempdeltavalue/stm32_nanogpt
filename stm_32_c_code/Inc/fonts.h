#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

#ifndef GFXFONT_DEFINED
#define GFXFONT_DEFINED
typedef struct {
    uint16_t bitmapOffset;
    uint8_t  width, height;
    uint8_t  xAdvance;
    int8_t   xOffset, yOffset;
} GFXglyph;

typedef struct {
    uint8_t  *bitmap;
    GFXglyph *glyph;
    uint16_t  first, last;
    uint8_t   yAdvance;
} GFXfont;
#endif

extern const GFXfont cambriaUkr8;

#endif
