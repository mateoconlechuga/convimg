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

#ifndef OUTPUT_H
#define OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "appvar.h"
#include "convert.h"
#include "palette.h"
#include "compress.h"

#include <stdint.h>

typedef enum
{
    OUTPUT_FORMAT_INVALID,
    OUTPUT_FORMAT_C,
    OUTPUT_FORMAT_ASM,
    OUTPUT_FORMAT_ICE,
    OUTPUT_FORMAT_APPVAR,
    OUTPUT_FORMAT_BIN
} output_format_t;

typedef struct
{
    char *name;
    char *includeFileName;
    char *directory;
    char **convertNames;
    convert_t **converts;
    int numConverts;
    char **paletteNames;
    palette_t **palettes;
    int numPalettes;
    output_format_t format;
    compress_t compress;
    appvar_t appvar;
} output_t;

output_t *output_alloc(void);
void output_free(output_t *output);
int output_add_convert(output_t *output, const char *convertName);
int output_add_palette(output_t *output, const char *paletteName);
int output_converts(output_t *output, convert_t **converts, int numConverts);
int output_palettes(output_t *output, palette_t **palettes, int numPalettes);
int output_include_header(output_t *output);

#ifdef __cplusplus
}
#endif

#endif
