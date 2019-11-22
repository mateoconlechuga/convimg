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

#include "output.h"
#include "tileset.h"
#include "strings.h"
#include "image.h"
#include "log.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

/*
 * Outputs to ICE format.
 */
static int output_ice(unsigned char *data, size_t size, FILE *fd)
{
    size_t i;

    fprintf(fd, "\"");

    for (i = 0; i < size; ++i)
    {
        fprintf(fd, "%02X", data[i]);
    }

    fprintf(fd, "\"\r\n\r\n");

    return 0;
}

/*
 * Outputs a converted C image.
 */
int output_ice_image(image_t *image, char *file)
{
    FILE *fd;

    fd = fopen(file, "a");
    if (fd == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        return 1;
    }

    fprintf(fd, "%s | %d bytes\r\n", image->name, image->size);
    output_ice(image->data, image->size, fd);

    fclose(fd);

    return 0;
}

/*
 * Outputs a converted ICE tileset.
 */
int output_ice_tileset(tileset_t *tileset, char *file)
{
    LL_ERROR("Tilesets are not supported for ICE output!");

    (void)tileset;
    (void)file;

    return 1;
}

/*
 * Outputs a converted C tileset.
 */
int output_ice_palette(palette_t *palette, char *file)
{
    int size = palette->numEntries * 2;
    FILE *fd;
    int i;

    fd = fopen(file, "a");
    if (fd == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        return 1;
    }

    fprintf(fd, "%s | %d bytes\r\n\"", palette->name, size);

    for (i = 0; i < palette->numEntries; ++i)
    {
        color_t *color = &palette->entries[i].color;

        fprintf(fd, "%02X%02X",
                color->target & 255,
                (color->target >> 8) & 255);
    }
    fprintf(fd, "\"\r\n\r\n");

    fclose(fd);

    return 0;
}

/*
 * Outputs an include file for the output structure
 */
int output_ice_include_file(output_t *output, char *file)
{
    LL_INFO(" - Wrote \'%s\'", file);

    (void)output;

    return 0;
}
