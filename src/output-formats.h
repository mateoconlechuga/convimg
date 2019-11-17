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

#ifndef OUTPUT_FORMATS_H
#define OUTPUT_FORMATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "output.h"

/*
 * Output is CRLF to please the Windows losers.
 */

/*
 * C Format.
 */
int output_c_image(image_t *image);
int output_c_tileset(tileset_t *tileset);
int output_c_palette(palette_t *palette);
int output_c_include_file(output_t *output);

/*
 * Assembly Format.
 */
int output_asm_image(image_t *image);
int output_asm_tileset(tileset_t *tileset);
int output_asm_palette(palette_t *palette);
int output_asm_include_file(output_t *output);

/*
 * ICE Format.
 */
int output_ice_image(image_t *image, char *file);
int output_ice_tileset(tileset_t *tileset, char *file);
int output_ice_palette(palette_t *palette, char *file);
int output_ice_include_file(output_t *output, char *file);

/*
 * Appvar Format.
 */
int output_appvar_image(image_t *image);
int output_appvar_tileset(tileset_t *tileset);
int output_appvar_palette(palette_t *palette);
int output_appvar_include_file(output_t *output);

/*
 * Binary Format.
 */
int output_bin_image(image_t *image);
int output_bin_tileset(tileset_t *tileset);
int output_bin_palette(palette_t *palette);
int output_bin_include_file(output_t *output);

#ifdef __cplusplus
}
#endif

#endif
