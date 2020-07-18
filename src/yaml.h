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

#ifndef YAML_H
#define YAML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "palette.h"
#include "convert.h"
#include "output.h"

#include <stdint.h>

typedef enum
{
    YAML_OUTPUT_CONVERTS,
    YAML_OUTPUT_PALETTES,
} yaml_output_mode_t;

typedef enum
{
    YAML_CONVERT_IMAGES,
    YAML_CONVERT_TILESETS,
} yaml_convert_mode_t;

typedef enum
{
    YAML_ST_INIT,
    YAML_ST_PALETTE,
    YAML_ST_CONVERT,
    YAML_ST_OUTPUT,
} yaml_state_t;

typedef struct
{
    char *name;
    palette_t **palettes;
    convert_t **converts;
    output_t **outputs;
    palette_t *curPalette;
    convert_t *curConvert;
    output_t *curOutput;
    int numPalettes;
    int numConverts;
    int numOutputs;
    yaml_state_t state;
    int line;
} yaml_file_t;

int yaml_parse_file(yaml_file_t *yamlfile);
void yaml_release_file(yaml_file_t *yamlfile);

#ifdef __cplusplus
}
#endif

#endif
