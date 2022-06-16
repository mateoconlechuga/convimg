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

#include "output.h"
#include "tileset.h"
#include "strings.h"
#include "image.h"
#include "log.h"
#include "clean.h"

#include <errno.h>
#include <string.h>

static int output_bin(unsigned char *data, size_t size, FILE *fdo)
{
    int ret = fwrite(data, size, 1, fdo);

    return ret == 1 ? 0 : 1;
}

int output_bin_image(struct image *image)
{
    char *source = strdupcat(image->directory, ".bin");
    FILE *fds;
    int ret;

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wb");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    ret = output_bin(image->data, image->size, fds);

    fclose(fds);

    free(source);

    return ret;

error:
    free(source);
    return -1;
}

int output_bin_tileset(struct tileset *tileset)
{
    char *source = strdupcat(tileset->directory, ".bin");
    FILE *fds;
    int i;

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wb");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    if (tileset->p_table == true)
    {
        int offset = tileset->nr_tiles * 3;

        for (i = 0; i < tileset->nr_tiles; ++i)
        {
            unsigned char tile_offset[3];

            tile_offset[0] = offset & 255;
            tile_offset[1] = (offset >> 8) & 255;
            tile_offset[2] = (offset >> 16) & 255;

            output_bin(tile_offset, sizeof tile_offset, fds);

            offset += tileset->tiles[i].size;
        }
    }

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        output_bin(tile->data, tile->size, fds);
    }

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_bin_palette(struct palette *palette)
{
    char *source = strdupcat(palette->directory, ".bin");
    FILE *fds;
    int i;

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wb");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    if (palette->include_size)
    {
        uint16_t size = palette->nr_entries * 2;
        fwrite(&size, sizeof(uint16_t), 1, fds);
    }

    for (i = 0; i < palette->nr_entries; ++i)
    {
        struct color *color = &palette->entries[i].color;

        fwrite(&color->target, sizeof(uint16_t), 1, fds);
    }

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_bin_include_file(struct output *output)
{
    char *include_file;
    char *include_name;
    char *tmp;
    FILE *fdi;
    int i, j, k;

    if (output->include_file == NULL)
    {
        return 0;
    }

    include_file = strdupcat(output->directory, output->include_file);
    include_name = strdup(output->include_file);

    tmp = strchr(include_name, '.');
    if (tmp != NULL)
    {
        *tmp = '\0';
    }

    LOG_INFO(" - Writing \'%s\'\n", include_file);

    fdi = clean_fopen(include_file, "wt");
    if (fdi == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    for (i = 0; i < output->nr_palettes; ++i)
    {
        fprintf(fdi, "%s.bin\n", output->palettes[i]->name);
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *tileset_group = convert->tileset_group;

        for (j = 0; j < convert->nr_images; ++j)
        {
            struct image *image = &convert->images[j];

            fprintf(fdi, "%s.bin\n", image->name);
        }

        if (tileset_group != NULL)
        {
            for (k = 0; k < tileset_group->nr_tilesets; ++k)
            {
                struct tileset *tileset = &tileset_group->tilesets[k];

                fprintf(fdi, "%s.bin\n", tileset->image.name);
            }
        }
    }

    fclose(fdi);

    free(include_name);
    free(include_file);

    return 0;

error:

    free(include_name);
    free(include_file);

    return -1;
}
