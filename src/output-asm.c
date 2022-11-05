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

static int output_asm_array(unsigned char *arr, uint32_t size, FILE *fdo)
{
    uint32_t i;

    for (i = 0; i < size; ++i)
    {
        bool last = i + 1 == size;

        if (last || ((i + 1) % 32 == 0))
        {
            fprintf(fdo, "$%02x", arr[i]);
            if (!last)
            {
                fputs("\n\tdb\t", fdo);
            }
        }
        else
        {
             fprintf(fdo, "$%02x,", arr[i]);
        }
    }

    fputc('\n', fdo);

    return 0;
}

int output_asm_image(struct output *output, const struct image *image)
{
    char *source = NULL;
    FILE *fds = NULL;

    source = strings_concat(output->directory, image->name, ".asm", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wt");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    fprintf(fds, "%s_width := %d\n", image->name, image->width);
    fprintf(fds, "%s_height := %d\n", image->name, image->height);
    fprintf(fds, "%s_size := %d\n", image->name, image->uncompressed_size);
    if (image->compressed)
    {
        fprintf(fds, "%s_compressed_size := %d\n", image->name, image->data_size);
    }
    fprintf(fds, "%s:\n\tdb\t", image->name);

    output_asm_array(image->data, image->data_size, fds);

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_asm_tileset(struct output *output, const struct tileset *tileset)
{
    char *source = NULL;
    FILE *fds = NULL;
    uint32_t i;

    source = strings_concat(output->directory, tileset->image.name, ".asm", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wt");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    fprintf(fds, "%s_num_tiles := %d\n",
        tileset->image.name,
        tileset->nr_tiles);

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        fprintf(fds, "%s_tile_%d:\n\tdb\t", tileset->image.name, i);

        output_asm_array(tile->data, tile->data_size, fds);
    }

    if (tileset->p_table == true)
    {
        fprintf(fds, "%s_tiles:\n", tileset->image.name);

        for (i = 0; i < tileset->nr_tiles; ++i)
        {
            fprintf(fds, "\tdl\t%s_tile_%d\n",
                tileset->image.name,
                i);
        }
    }

    fclose(fds);
    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_asm_palette(struct output *output, const struct palette *palette)
{
    char *source = NULL;
    FILE *fds = NULL;
    uint32_t size;
    uint32_t i;

    source = strings_concat(output->directory, palette->name, ".asm", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wt");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    size = palette->nr_entries * 2;

    fprintf(fds, "sizeof_%s := %d\n", palette->name, size);

    if (output->palette_sizes)
    {
        fprintf(fds, "\tdw\t%d\n", size);
    }

    fprintf(fds, "%s:\n", palette->name);

    for (i = 0; i < palette->nr_entries; ++i)
    {
        const struct palette_entry *entry = &palette->entries[i];
        const struct color *color = &entry->color;
        const uint16_t target = entry->target;

        if (entry->valid)
        {
            fprintf(fds, "\tdw\t$%04x ; %3d: rgb(%3d, %3d, %3d)\n",
                    target,
                    i,
                    color->r,
                    color->g,
                    color->b);
        }
        else
        {
            fprintf(fds, "\tdw\t$0000 ; %3d: unused\n", i);
        }
    }

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return -1;
}

int output_asm_include(struct output *output)
{
    char *include_name = NULL;
    char *tmp = NULL;
    FILE *fdi = NULL;
    uint32_t i;

    include_name = strings_dup(output->include_file);
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
        fprintf(fdi, "include \'%s.asm\'\n", output->palettes[i]->name);
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *tileset_group = convert->tileset_group;
        uint32_t j;

        for (j = 0; j < convert->nr_images; ++j)
        {
            struct image *image = &convert->images[j];

            fprintf(fdi, "include \'%s.asm\'\n", image->name);
        }

        if (tileset_group != NULL)
        {
            uint32_t k;

            for (k = 0; k < tileset_group->nr_tilesets; ++k)
            {
                struct tileset *tileset = &tileset_group->tilesets[k];

                fprintf(fdi, "include \'%s.asm\'\n", tileset->image.name);
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

int output_asm_init(struct output *output)
{
    (void)output;
    
    return 0;
}
