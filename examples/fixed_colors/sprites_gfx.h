// convpng v7.0
// this file contains all the graphics sources for easy inclusion in a project
#ifndef __sprites_gfx__
#define __sprites_gfx__
#include <stdint.h>

#define sprites_gfx_transparent_color_index 0

#define sprite_width 16
#define sprite_height 16
#define sprite_size 258
extern uint8_t sprite_data[258];
#define sprite ((gfx_sprite_t*)sprite_data)
#define sizeof_sprites_gfx_pal 62
extern uint16_t sprites_gfx_pal[31];

#endif
