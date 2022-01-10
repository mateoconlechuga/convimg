/*
 * Copyright 2017-2021 Matt "MateoConLechuga" Waltz
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
#include "compress.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct palette;

struct image
{
    char *name;
    char *path;
    uint8_t *data;
    int width;
    int height;
    int size;

    // set by convert
    int quantize_speed;
    float dither;
    bool compressed;
    bool rlet;
    int rotate;
    bool flip_x;
    bool flip_y;
    int orig_size;
    bool add_tcp;

    // set by output
    char *directory;
};

#define WIDTH_HEIGHT_SIZE 2

int image_load(struct image *image);

int image_rlet(struct image *image, int transparent_index);

int image_add_width_and_height(struct image *image);

int image_add_offset(struct image *image, int offset);

int image_add_tcp(struct image *image);

int image_compress(struct image *image, compress_t compress);

int image_remove_omits(struct image *image, int *omit_indices, int nr_omit_indices);

int image_set_bpp(struct image *image, bpp_t bpp, int palette_nr_entries);

int image_quantize(struct image *image, struct palette *palette);

void image_free(struct image *image);

#ifdef __cplusplus
}
#endif

#endif
