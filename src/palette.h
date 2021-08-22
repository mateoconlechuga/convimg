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

#ifndef PALETTE_H
#define PALETTE_H

#include "bpp.h"
#include "image.h"
#include "color.h"

#include "deps/libimagequant/libimagequant.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PALETTE_MAX_ENTRIES 256
#define PALETTE_DEFAULT_QUANTIZE_SPEED 3

struct convert;

struct palette_entry
{
    struct color color;
    struct color orig_color;
    unsigned int index;
    bool exact;
    bool valid;
    bool fixed;
};

struct palette
{
    char *name;
    struct image *images;
    int nr_images;
    int max_entries;
    int nr_entries;
    int nr_fixed_entries;
    int quantize_speed;
    struct palette_entry entries[PALETTE_MAX_ENTRIES];
    struct palette_entry fixed_entries[PALETTE_MAX_ENTRIES];
    color_mode_t mode;
    bpp_t bpp;
    bool automatic;

    // set by output
    char *directory;
    bool include_size;
};

struct palette *palette_alloc(void);

void palette_free(struct palette *palette);

int pallete_add_path(struct palette *palette,
    const char *path);

int palette_generate(struct palette *palette,
    struct convert **converts,
    int nr_converts);

#ifdef __cplusplus
}
#endif

#endif
