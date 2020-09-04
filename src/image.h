/*
 * Copyright 2017-2020 Matt "MateoConLechuga" Waltz
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

#ifdef __cplusplus
extern "C" {
#endif

#include "compress.h"
#include "bpp.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    char *name;
    char *path;
    uint8_t *data;
    int width;
    int height;
	int size;

    /* set by convert */
    int quantizeSpeed;
    float dither;
    bool compressed;
    bool rlet;
    int rotate;
    bool flipx;
    bool flipy;
    int origSize;

    /* set by output */
    char *directory;
} image_t;

#define WIDTH_HEIGHT_SIZE 2

/* I despise forward declartions, but meh */
typedef struct palette palette_t;

int image_load(image_t *image);
int image_rlet(image_t *image, int tIndex);
int image_add_width_and_height(image_t *image);
int image_compress(image_t *image, compress_t compress);
int image_remove_omits(image_t *image, int *omitIndices, int numOmitIndices);
int image_set_bpp(image_t *image, bpp_t bpp, int paletteNumEntries);
int image_quantize(image_t *image, palette_t *palette);
void image_free(image_t *image);

#ifdef __cplusplus
}
#endif

#endif
