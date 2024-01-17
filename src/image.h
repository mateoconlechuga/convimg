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

#ifndef IMAGE_H
#define IMAGE_H

#include "bpp.h"
#include "color.h"
#include "compress.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct palette;

struct image
{
    /* assigned on init */
    char *name;
    char *path;
    uint8_t *data;
    uint32_t data_size;
    uint32_t uncompressed_size;
    uint32_t width;
    uint32_t height;

    /* set by convert */
    uint8_t transparent_index;
    uint32_t quantize_speed;
    uint32_t rotate;
    bool gfx;
    bool compressed;
    bool rlet;
    bool flip_x;
    bool flip_y;
    float dither;
};

#define WIDTH_HEIGHT_SIZE 2

void image_init(struct image *image, const char *path);

int image_load(struct image *image);

int image_rlet(struct image *image, uint8_t transparent_index);

int image_add_width_and_height(struct image *image);

int image_add_offset(struct image *image, uint8_t offset);

int image_compress(struct image *image, compress_mode_t mode);

int image_remove_omits(struct image *image, const uint8_t *omit_indices, uint32_t nr_omit_indices);

int image_set_bpp(struct image *image, bpp_t bpp, uint32_t palette_nr_entries);

int image_quantize(struct image *image, const struct palette *palette);

int image_direct_convert(struct image *image, color_format_t fmt);

void image_free(struct image *image);

#ifdef __cplusplus
}
#endif

#endif
