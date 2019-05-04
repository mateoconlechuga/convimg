// convpng v7.0
// this file contains all the graphics sources for easy inclusion in a project
#ifndef __sprites_gfx__
#define __sprites_gfx__
#include <stdint.h>

#define sprites_gfx_transparent_color_index 2

#define sprite_1_width 16
#define sprite_1_height 16
#define sprite_1_size 258
extern uint8_t sprite_1_data[258];
#define sprite_1 ((gfx_sprite_t*)sprite_1_data)
#define sprite_2_width 16
#define sprite_2_height 16
#define sprite_2_size 258
extern uint8_t sprite_2_data[258];
#define sprite_2 ((gfx_sprite_t*)sprite_2_data)
#define sizeof_sprites_gfx_pal 512
extern uint16_t sprites_gfx_pal[256];

#endif
