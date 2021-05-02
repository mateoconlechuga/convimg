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
#ifndef CONVERT_H
#define CONVERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bpp.h"
#include "image.h"
#include "palette.h"
#include "tileset.h"
#include "compress.h"

#define CONVERT_DEFAULT_QUANTIZE_SPEED 3

typedef enum
{
    CONVERT_STYLE_NORMAL,
    CONVERT_STYLE_RLET,
} convert_style_t;

typedef struct convert
{
    char *name;
    char *paletteName;
    int paletteOffset;
    image_t *images;
    int numImages;
    tileset_group_t *tilesetGroup;
    compress_t compress;
    palette_t *palette;
    convert_style_t style;
    int omitIndices[PALETTE_MAX_ENTRIES];
    int numOmitIndices;
    int transparentIndex;
    bool widthAndHeight;
    int quantizeSpeed;
    float dither;
    int rotate;
    bool flipx;
    bool flipy;
    bpp_t bpp;
} convert_t;

convert_t *convert_alloc(void);
void convert_free(convert_t *convert);
tileset_group_t *convert_alloc_tileset_group(convert_t *convert);
int convert_add_image_path(convert_t *convert, const char *path);
int convert_add_tileset_path(convert_t *convert, const char *path);
int convert_convert(convert_t *convert, palette_t **palettes, int numPalettes);

#ifdef __cplusplus
}
#endif

#endif
