/*
 * Copyright 2017-2019 Matt "MateoConLechuga" Waltz
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

#ifndef PALETTE_H
#define PALETTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bpp.h"
#include "image.h"
#include "color.h"

#include "deps/libimagequant/libimagequant.h"

#include <stdint.h>
#include <stdbool.h>

#define PALETTE_MAX_ENTRIES 256
#define PALETTE_DEFAULT_QUANTIZE_SPEED 3

typedef struct
{
    color_t color;
    unsigned int index;
} palette_entry_t;

typedef struct palette
{
    char *name;
    image_t *images;
    int numImages;
    int maxEntries;
    int numEntries;
    int numFixedEntries;
    int quantizeSpeed;
    palette_entry_t entries[PALETTE_MAX_ENTRIES];
    palette_entry_t fixedEntries[PALETTE_MAX_ENTRIES];
    color_mode_t mode;
    bpp_t bpp;
    bool automatic;

    /* set by output */
    char *directory;
} palette_t;

/* I despise forward declartions, but meh */
typedef struct convert convert_t;

palette_t *palette_alloc(void);
void palette_free(palette_t *palette);
int pallete_add_path(palette_t *palette, const char *path);
int palette_generate(palette_t *palette, convert_t **converts, int numConverts);

#ifdef __cplusplus
}
#endif

#endif
