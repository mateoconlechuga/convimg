#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "libs/libimagequant.h"

void *safe_malloc(size_t n);
void *safe_realloc(void *a, size_t n);
void *safe_calloc(size_t n, size_t m);
int create_icon(void);
char *str_dup(const char *s);
char *str_dupcat(const char *s, const char *c);
uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8);
uint16_t rgb565(const uint8_t r8, const uint8_t g8, const uint8_t b8);
void encodePNG(const char* filename, const unsigned char* image, unsigned width, unsigned height);
void build_image_palette(const liq_palette *pal, const unsigned length, const char *filename);

#endif
