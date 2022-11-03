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

static int output_bin_array(unsigned char *data, size_t size, FILE *fdo)
{
    int ret = fwrite(data, size, 1, fdo);

    return ret == 1 ? 0 : 1;
}

int output_bin_image(struct output *output, struct image *image)
{
    char *source;
    FILE *fds;
    int ret;

    source = strings_concat(output->directory, image->name, ".bin", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wb");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    ret = output_bin_array(image->data, image->size, fds);

    fclose(fds);

    free(source);

    return ret;

error:
    free(source);
    return -1;
}

int output_bin_tileset(struct output *output, struct tileset *tileset)
{
    char *source;
    FILE *fds;
    int i;

    source = strings_concat(output->directory, tileset->image.name, ".bin", NULL);
    if (source == NULL)
    {
        goto error;
    }

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

            output_bin_array(tile_offset, sizeof tile_offset, fds);

            offset += tileset->tiles[i].size;
        }
    }

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        output_bin_array(tile->data, tile->size, fds);
    }

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_bin_palette(struct output *output, struct palette *palette)
{
    char *source;
    FILE *fds;
    int i;

    source = strings_concat(output->directory, palette->name, ".bin", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wb");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    if (output->palette_sizes)
    {
        uint16_t size = palette->nr_entries * 2;

        fputc(size & 255, fds);
        fputc((size >> 8) & 255, fds);
    }

    for (i = 0; i < palette->nr_entries; ++i)
    {
        uint16_t target = palette->entries[i].target;

        fputc(target & 255, fds);
        fputc((target >> 8) & 255, fds);
    }

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_bin_include(struct output *output)
{
    char *include_name = NULL;
    char *tmp;
    FILE *fdi;
    int i, j, k;

    if (output->include_file == NULL)
    {
        return 0;
    }

    include_name = strdup(output->include_file);
    if (include_name == NULL)
    {
        goto error;
    }

    tmp = strchr(include_name, '.');
    if (tmp != NULL)
    {
        *tmp = '\0';
    }

    LOG_INFO(" - Writing \'%s\'\n", output->include_file);

    fdi = clean_fopen(output->include_file, "wt");
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

    return 0;

error:
    free(include_name);
    return -1;
}

int output_bin_init(struct output *output)
{
    (void)output;
    
    return 0;
}