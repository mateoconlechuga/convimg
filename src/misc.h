#ifndef MISC_H
#define MISC_H

#include "libs/libimagequant.h"

int create_icon(void);
char *str_dup(const char *s);
uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8);
uint16_t rgb565(const uint8_t r8, const uint8_t g8, const uint8_t b8);
void encodePNG(const char* filename, const unsigned char* image, unsigned width, unsigned height);
void build_image_palette(const liq_palette *pal, const unsigned length, const char *filename);

#endif