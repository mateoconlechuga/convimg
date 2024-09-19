/*
 * Copyright 2017-2024 Matt "MateoConLechuga" Waltz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef CONVERT_H
#define CONVERT_H

#include "bpp.h"
#include "image.h"
#include "palette.h"
#include "tileset.h"
#include "compress.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONVERT_DEFAULT_QUANTIZE_SPEED 3

typedef enum
{
    CONVERT_STYLE_PALETTE,
    CONVERT_STYLE_RLET,
    CONVERT_STYLE_DIRECT,
} convert_style_t;

struct convert
{
    char *name;
    char *palette_name;
    char *append_string;
    const struct palette *palette;
    uint8_t palette_offset;
    uint8_t omit_indices[PALETTE_MAX_ENTRIES];
    uint32_t nr_omit_indices;
    uint8_t transparent_index;
    struct image *images;
    uint32_t nr_images;
    struct tileset *tilesets;
    uint32_t nr_tilesets;
    uint32_t tile_height;
    uint32_t tile_width;
    bool p_table;
    compress_mode_t compress;
    convert_style_t style;
    color_format_t color_fmt;
    uint32_t quantize_speed;
    float dither;
    uint32_t rotate;
    bool add_width_height;
    bool flip_x;
    bool flip_y;
    uint32_t tile_rotate;
    bool tile_flip_x;
    bool tile_flip_y;
    bpp_t bpp;
};

struct convert *convert_alloc(void);

int convert_add_image_path(struct convert *convert, const char *path);

int convert_add_tileset_path(struct convert *convert, const char *path);

int convert_generate(struct convert *convert, struct palette **palettes, uint32_t nr_palettes);

void convert_free(struct convert *convert);

#ifdef __cplusplus
}
#endif

#endif
