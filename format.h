#ifndef FORMAT_H
#define FORMAT_H

#include "main.h"

extern const format_t c_format;
extern const format_t asm_format;
extern const format_t ice_format;

void ice_print_tilemap_header(output_t *out, const char *name, unsigned int size, unsigned int tile_width, unsigned int tile_height);

#endif
