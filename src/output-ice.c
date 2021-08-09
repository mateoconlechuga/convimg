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

#include "output.h"
#include "tileset.h"
#include "strings.h"
#include "image.h"
#include "log.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

static int output_ice(unsigned char *data, size_t size, FILE *fd)
{
    size_t i;

    fprintf(fd, "\"");

    for (i = 0; i < size; ++i)
    {
        fprintf(fd, "%02X", data[i]);
    }

    fprintf(fd, "\"\n\n");

    return 0;
}

int output_ice_image(struct image *image, char *file)
{
    FILE *fd;

    fd = fopen(file, "at");
    if (fd == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        return -1;
    }

    fprintf(fd, "%s | %d bytes\n", image->name, image->size);
    output_ice(image->data, image->size, fd);

    fclose(fd);

    return 0;
}

int output_ice_tileset(struct tileset *tileset, char *file)
{
    LOG_ERROR("Tilesets are not yet supported for ICE output!\n");

    (void)tileset;
    (void)file;

    return -1;
}

int output_ice_palette(struct palette *palette, char *file)
{
    int size = palette->nr_entries * 2;
    FILE *fd;
    int i;

    fd = fopen(file, "at");
    if (fd == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        return -1;
    }

    fprintf(fd, "%s | %d bytes\n\"", palette->name, size);

    for (i = 0; i < palette->nr_entries; ++i)
    {
        struct color *c = &palette->entries[i].color;

        fprintf(fd, "%02X%02X",
                c->target & 255,
                (c->target >> 8) & 255);
    }
    fprintf(fd, "\"\n\n");

    fclose(fd);

    return 0;
}

int output_ice_include_file(struct output *output, char *file)
{
    LOG_INFO(" - Wrote \'%s\'\n", file);

    (void)output;

    return 0;
}
