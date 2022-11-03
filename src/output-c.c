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
#include "version.h"

#include <errno.h>
#include <string.h>

static int output_c_array(unsigned char *arr, size_t size, FILE *fdo)
{
    size_t i;

    for (i = 0; i < size; ++i)
    {
        if (i % 32 == 0)
        {
            fputs("\n    ", fdo);
        }

        if (i + 1 == size)
        {
            fprintf(fdo, "0x%02x", arr[i]);
        }
        else
        {
            fprintf(fdo, "0x%02x,", arr[i]);
        }
    }

    fprintf(fdo, "\n};\n");

    return 0;
}

int output_c_image(struct output *output, struct image *image)
{
    char *header = NULL;
    char *source = NULL;
    FILE *fdh;
    FILE *fds;

    header = strings_concat(output->directory, image->name, ".h", NULL);
    if (header == NULL)
    {
        goto error;
    }

    source = strings_concat(output->directory, image->name, ".c", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", header);

    fdh = clean_fopen(header, "wt");
    if (fdh == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    fprintf(fdh, "#ifndef %s_include_file\n", image->name);
    fprintf(fdh, "#define %s_include_file\n", image->name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#define %s_width %d\n", image->name, image->width);
    fprintf(fdh, "#define %s_height %d\n", image->name, image->height);
    fprintf(fdh, "#define %s_size %d\n", image->name, image->size);

    if (image->compressed)
    {
        fprintf(fdh, "#define %s_compressed_size %d\n", image->name, image->compressed_size);
        fprintf(fdh, "extern %sunsigned char %s_compressed[%d];\n",
            output->constant, image->name, image->size);
    }
    else
    {
        if (image->gfx)
        {
            fprintf(fdh, "#define %s ((%s%s*)%s_data)\n",
                image->name,
                output->constant,
                image->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                image->name);
        }

        fprintf(fdh, "extern %sunsigned char %s_data[%d];\n",
            output->constant, image->name, image->size);
    }

    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");

    fclose(fdh);

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wt");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    if (image->compressed)
    {
        fprintf(fds, "%sunsigned char %s_compressed[%d] =\n{",
            output->constant, image->name, image->size);
    }
    else
    {
        fprintf(fds, "%sunsigned char %s_data[%d] =\n{",
            output->constant, image->name, image->size);
    }

    output_c_array(image->data, image->size, fds);

    fclose(fds);

    free(header);
    free(source);

    return 0;

error:
    free(header);
    free(source);
    return -1;
}

int output_c_tileset(struct output *output, struct tileset *tileset)
{
    char *header = NULL;
    char *source = NULL;
    FILE *fdh;
    FILE *fds;
    int i;

    header = strings_concat(output->directory, tileset->image.name, ".h", NULL);
    if (header == NULL)
    {
        goto error;
    }

    source = strings_concat(output->directory, tileset->image.name, ".c", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", header);

    fdh = clean_fopen(header, "wt");
    if (fdh == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    fprintf(fdh, "#ifndef %s_include_file\n", tileset->image.name);
    fprintf(fdh, "#define %s_include_file\n", tileset->image.name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        if (tileset->compressed)
        {
            fprintf(fdh, "extern %sunsigned char %s_tile_%d_compressed[%d];\n",
                output->constant,
                tileset->image.name,
                i,
                tile->size);
        }
        else
        {
            if (tileset->image.gfx)
            {
                fprintf(fdh, "#define %s_tile_%d ((%s%s*)%s_tile_%d_data)\n",
                    tileset->image.name,
                    i,
                    output->constant,
                    tileset->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                    tileset->image.name,
                    i);
            }

            fprintf(fdh, "extern %sunsigned char %s_tile_%d_data[%d];\n",
                output->constant,
                tileset->image.name,
                i,
                tile->size);
        }
    }

    fprintf(fdh, "#define %s_num_tiles %d\n",
        tileset->image.name,
        tileset->nr_tiles);

    if (tileset->p_table)
    {
        if (tileset->compressed)
        {
            fprintf(fdh, "extern %sunsigned char *%s_tiles_compressed[%d];\n",
                output->constant,
                tileset->image.name,
                tileset->nr_tiles);
        }
        else
        {
            if (tileset->image.gfx)
            {
                fprintf(fdh, "#define %s_tiles ((%s%s**)%s_tiles_data)\n",
                    tileset->image.name,
                    output->constant,
                    tileset->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                    tileset->image.name);
            }
            
            fprintf(fdh, "extern %sunsigned char *%s_tiles_data[%d];\n",
                output->constant,
                tileset->image.name,
                tileset->nr_tiles);
        }
    }

    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");

    fclose(fdh);

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wt");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        if (tileset->compressed)
        {
            fprintf(fds, "%sunsigned char %s_tile_%d_compressed[%d] =\n{",
                output->constant,
                tileset->image.name,
                i,
                tile->size);
        }
        else
        {
            fprintf(fds, "%sunsigned char %s_tile_%d_data[%d] =\n{",
                output->constant,
                tileset->image.name,
                i,
                tile->size);
        }

        output_c_array(tile->data, tile->size, fds);
    }

    if (tileset->p_table)
    {
        if (tileset->compressed)
        {
            fprintf(fds, "%sunsigned char *%s_tiles_compressed[%d] =\n{\n",
                output->constant,
                tileset->image.name,
                tileset->nr_tiles);
        }
        else
        {
            fprintf(fds, "%sunsigned char *%s_tiles_data[%d] =\n{\n",
                output->constant,
                tileset->image.name,
                tileset->nr_tiles);
        }

        for (i = 0; i < tileset->nr_tiles; ++i)
        {
            if (tileset->compressed)
            {
                fprintf(fds, "    %s_tile_%d_compressed,\n",
                    tileset->image.name,
                    i);
            }
            else
            {
                fprintf(fds, "    %s_tile_%d_data,\n",
                    tileset->image.name,
                    i);
            }
        }

        fprintf(fds, "};\n");
    }

    fclose(fds);

    free(header);
    free(source);

    return 0;

error:
    free(header);
    free(source);
    return -1;
}

int output_c_palette(struct output *output, struct palette *palette)
{
    char *header = NULL;
    char *source = NULL;
    FILE *fdh;
    FILE *fds;
    int size;
    int i;

    header = strings_concat(output->directory, palette->name, ".h", NULL);
    if (header == NULL)
    {
        goto error;
    }

    source = strings_concat(output->directory, palette->name, ".c", NULL);
    if (source == NULL)
    {
        goto error;
    }

    LOG_INFO(" - Writing \'%s\'\n", header);

    fdh = clean_fopen(header, "wt");
    if (fdh == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    size = palette->nr_entries * sizeof(uint16_t);

    fprintf(fdh, "#ifndef %s_include_file\n", palette->name);
    fprintf(fdh, "#define %s_include_file\n", palette->name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#define sizeof_%s %d\n", palette->name, size);
    fprintf(fdh, "extern %sunsigned char %s[%d];\n",
        output->constant, palette->name, size);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");

    fclose(fdh);

    LOG_INFO(" - Writing \'%s\'\n", source);

    fds = clean_fopen(source, "wt");
    if (fds == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    fprintf(fds, "%sunsigned char %s[%d] =\n{\n",
        output->constant, palette->name, size);

    for (i = 0; i < palette->nr_entries; ++i)
    {
        struct palette_entry *entry = &palette->entries[i];
        struct color *color = &entry->color;
        struct color *origcolor = &entry->orig_color;
        uint16_t target = entry->target;

        if (entry->exact)
        {
            fprintf(fds, "    0x%02x, 0x%02x, /* %3d: rgb(%3d, %3d, %3d) [exact original: rgb(%3d, %3d, %3d)] */\n",
                    target & 255,
                    (target >> 8) & 255,
                    i,
                    color->r, color->g, color->b,
                    origcolor->r, origcolor->g, origcolor->b);
        }
        else if (!entry->valid)
        {
            fprintf(fds, "    0x00, 0x00, /* %3d: (unused) */\n", i);
        }
        else
        {
            fprintf(fds, "    0x%02x, 0x%02x, /* %3d: rgb(%3d, %3d, %3d) */\n",
                    target & 255,
                    (target >> 8) & 255,
                    i,
                    color->r, color->g, color->b);

        }
    }
    fprintf(fds, "};\n");

    fclose(fds);

    free(header);
    free(source);

    return 0;

error:
    free(header);
    free(source);
    return -1;
}

int output_c_include(struct output *output)
{
    char *include_name = NULL;
    char *tmp;
    FILE *fdi;
    int i;

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

    fprintf(fdi, "#ifndef %s_include_file\n", include_name);
    fprintf(fdi, "#define %s_include_file\n", include_name);
    fprintf(fdi, "\n");
    fprintf(fdi, "#ifdef __cplusplus\n");
    fprintf(fdi, "extern \"C\" {\n");
    fprintf(fdi, "#endif\n");
    fprintf(fdi, "\n");

    for (i = 0; i < output->nr_palettes; ++i)
    {
        fprintf(fdi, "#include \"%s.h\"\n", output->palettes[i]->name);
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *tileset_group = convert->tileset_group;
        int j;

        fprintf(fdi, "#define %s_palette_offset %d\n",
            convert->name, convert->palette_offset);

        for (j = 0; j < convert->nr_images; ++j)
        {
            struct image *image = &convert->images[j];

            fprintf(fdi, "#include \"%s.h\"\n", image->name);
        }

        if (tileset_group != NULL)
        {
            int k;

            for (k = 0; k < tileset_group->nr_tilesets; ++k)
            {
                struct tileset *tileset = &tileset_group->tilesets[k];

                fprintf(fdi, "#include \"%s.h\"\n", tileset->image.name);
            }
        }
    }

    fprintf(fdi, "\n");
    fprintf(fdi, "#ifdef __cplusplus\n");
    fprintf(fdi, "}\n");
    fprintf(fdi, "#endif\n");
    fprintf(fdi, "\n");
    fprintf(fdi, "#endif\n");

    fclose(fdi);

    free(include_name);

    return 0;

error:
    free(include_name);
    return -1;
}

int output_c_init(struct output *output)
{
    (void)output;

    return 0;
}