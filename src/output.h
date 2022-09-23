/*
 * Copyright 2017-2022 Matt "MateoConLechuga" Waltz
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

#ifndef OUTPUT_H
#define OUTPUT_H

#include "appvar.h"
#include "convert.h"
#include "palette.h"
#include "compress.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    OUTPUT_FORMAT_INVALID,
    OUTPUT_FORMAT_C,
    OUTPUT_FORMAT_ASM,
    OUTPUT_FORMAT_ICE,
    OUTPUT_FORMAT_APPVAR,
    OUTPUT_FORMAT_BIN
} output_format_t;

typedef enum
{
    OUTPUT_PALETTES_FIRST,
    OUTPUT_CONVERTS_FIRST,
} output_order_t;

struct output
{
    char *name;
    char *include_file;
    char *directory;
    char **convert_names;
    struct convert **converts;
    int nr_converts;
    char **palette_names;
    struct palette **palettes;
    int nr_palettes;
    output_format_t format;
    bool constant;
    bool palette_sizes;
    compress_t compress;
    struct appvar appvar;
    output_order_t order;
};

struct output *output_alloc(void);

void output_free(struct output *output);

int output_find_palettes(struct output *output,
    struct palette **palettes,
    int nr_palettes);

int output_find_converts(struct output *output,
    struct convert **converts,
    int nr_converts);

int output_add_convert(struct output *output,
    const char *name);

int output_add_palette(struct output *output,
    const char *name);

int output_converts(struct output *output,
    struct convert **converts,
    int nr_converts);

int output_palettes(struct output *output,
    struct palette **palettes,
    int nr_palettes);

int output_include_header(struct output *output);

int output_init(struct output *output);

#ifdef __cplusplus
}
#endif

#endif
