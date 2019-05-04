// convpng v7.1
// this file contains all the graphics sources for easy inclusion in a project
#ifndef __var_gfx__
#define __var_gfx__
#include <stdint.h>

#include <stdbool.h>

#define var_gfx_num 3

extern uint8_t *var_gfx[3];
#define sprite_1_width 16
#define sprite_1_height 16
#define sprite_1 ((gfx_sprite_t*)var_gfx[0])
#define sprite_2_width 16
#define sprite_2_height 16
#define sprite_2 ((gfx_sprite_t*)var_gfx[1])
#define sizeof_all_gfx_pal 28
#define all_gfx_pal ((uint16_t*)var_gfx[2])
bool var_gfx_init(void *decompressed_addr);
#define var_gfx_uncompressed_size 544
#define var_gfx_compressed_size 157

#endif
