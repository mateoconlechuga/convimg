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
#include "appvar.h"
#include "strings.h"
#include "log.h"

#include <string.h>
#include <errno.h>

static int validate_data_size(struct appvar *appvar, int adding)
{
    if (appvar->size + adding >= APPVAR_MAX_BEFORE_COMPRESSION_SIZE)
    {
        LOG_ERROR("Too much input data to create AppVar \'%s\'.\n", appvar->name);
        return -1;
    }

    return 0;
}

int appvar_insert_entry(struct appvar *appvar, int index, int data)
{
    uint8_t *entry_table;
    uint8_t *entry;

    if (index > appvar->total_entries - 1)
    {
        LOG_ERROR("Invalid index for LUT entry.\n");
        return -1;
    }

    entry_table = &appvar->data[appvar->header_size];
    entry = entry_table + index * appvar->entry_size;

    if (appvar->entry_size == 2)
    {
        entry[0] = data & 0xff;
        entry[1] = data >> 8 & 0xff;
    }
    else if (appvar->entry_size == 3)
    {
        entry[0] = data & 0xff;
        entry[1] = data >> 8 & 0xff;
        entry[2] = data >> 16 & 0xff;
    }
    else
    {
        return -1;
    }

    return 0;
}

int output_appvar_header(struct output *output, struct appvar *appvar)
{
    int nr_entries;
    int i, j, k, l;
    int offset;
    int index;

    if (appvar->header_size > 0)
    {
        if (validate_data_size(appvar, appvar->header_size) != 0)
        {
            return -1;
        }

        memcpy(&appvar->data[appvar->size], appvar->header, appvar->header_size);
        appvar->size += appvar->header_size;
    }

    if (appvar->lut == false)
    {
        appvar->data_offset = appvar->header_size;
        return 0;
    }

    nr_entries = 0;

    for (i = 0; i < output->nr_palettes; ++i)
    {
        nr_entries++;
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *tileset_group = convert->tileset_group;

        for (j = 0; j < convert->nr_images; ++j)
        {
            nr_entries++;
        }

        if (tileset_group != NULL)
        {
            for (k = 0; k < tileset_group->nr_tilesets; ++k)
            {
                struct tileset *tileset = &tileset_group->tilesets[k];

                nr_entries++;

                for (l = 0; l < tileset->nr_tiles; l++)
                {
                    nr_entries++;
                }
            }
        }
    }

    // + 1 for lut size
    nr_entries += 1;
    appvar->data_offset = nr_entries * appvar->entry_size;
    appvar->total_entries = nr_entries;

    // temporary, just used to validate
    appvar->nr_entries = appvar->total_entries;

    if (validate_data_size(appvar, appvar->data_offset) != 0)
    {
        return -1;
    }

    appvar->size += appvar->data_offset;

    // first entry is number of entries
    appvar_insert_entry(appvar, 0, nr_entries - 1);

    offset = appvar->data_offset;
    index = 1;

    if (output->order == OUTPUT_PALETTES_FIRST)
    {
        for (i = 0; i < output->nr_palettes; ++i)
        {
            appvar_insert_entry(appvar, index, offset);

            offset += output->palettes[i]->nr_entries * 2;

            index++;
        }

        for (i = 0; i < output->nr_converts; ++i)
        {
            struct convert *convert = output->converts[i];
            struct tileset_group *tileset_group = convert->tileset_group;

            for (j = 0; j < convert->nr_images; ++j)
            {
                appvar_insert_entry(appvar, index, offset);

                offset += convert->images[j].size;

                index++;
            }

            if (tileset_group != NULL)
            {
                for (k = 0; k < tileset_group->nr_tilesets; ++k)
                {
                    struct tileset *tileset = &tileset_group->tilesets[k];
                    int tileset_offset = 0;

                    appvar_insert_entry(appvar, index, offset);

                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        tileset_offset += tileset->tiles[l].size;
                    }

                    offset += tileset_offset;

                    index++;
                }
            }
        }
        for (i = 0; i < output->nr_converts; ++i)
        {
            struct convert *convert = output->converts[i];
            struct tileset_group *tileset_group = convert->tileset_group;

            if (tileset_group != NULL)
            {
                for (k = 0; k < tileset_group->nr_tilesets; ++k)
                {
                    struct tileset *tileset = &tileset_group->tilesets[k];
                    int tileset_offset = 0;

                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        appvar_insert_entry(appvar, index, tileset_offset);

                        tileset_offset += tileset->tiles[l].size;

                        index++;
                    }
                }
            }
        }
    }
    else
    {
        for (i = 0; i < output->nr_converts; ++i)
        {
            struct convert *convert = output->converts[i];
            struct tileset_group *tileset_group = convert->tileset_group;

            for (j = 0; j < convert->nr_images; ++j)
            {
                appvar_insert_entry(appvar, index, offset);

                offset += convert->images[j].size;

                index++;
            }

            if (tileset_group != NULL)
            {
                for (k = 0; k < tileset_group->nr_tilesets; ++k)
                {
                    struct tileset *tileset = &tileset_group->tilesets[k];
                    int tileset_offset = 0;

                    appvar_insert_entry(appvar, index, offset);

                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        tileset_offset += tileset->tiles[l].size;
                    }

                    offset += tileset_offset;

                    index++;
                }
            }
        }
        for (i = 0; i < output->nr_converts; ++i)
        {
            struct convert *convert = output->converts[i];
            struct tileset_group *tileset_group = convert->tileset_group;

            if (tileset_group != NULL)
            {
                for (k = 0; k < tileset_group->nr_tilesets; ++k)
                {
                    struct tileset *tileset = &tileset_group->tilesets[k];
                    int tileset_offset = 0;

                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        appvar_insert_entry(appvar, index, tileset_offset);

                        tileset_offset += tileset->tiles[l].size;

                        index++;
                    }
                }
            }
        }

        for (i = 0; i < output->nr_palettes; ++i)
        {
            appvar_insert_entry(appvar, index, offset);

            offset += output->palettes[i]->nr_entries * 2;

            index++;
        }
    }

    appvar->data_offset += appvar->header_size;

    return 0;
}

int output_appvar_image(struct image *image, struct appvar *appvar)
{
    if (validate_data_size(appvar, image->size) != 0)
    {
        return -1;
    }

    memcpy(&appvar->data[appvar->size], image->data, image->size);
    appvar->size += image->size;

    return 0;
}

int output_appvar_tileset(struct tileset *tileset, struct appvar *appvar)
{
    int i;

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        if (validate_data_size(appvar, tile->size) != 0)
        {
            return -1;
        }

        memcpy(&appvar->data[appvar->size], tile->data, tile->size);
        appvar->size += tile->size;
    }

    return 0;
}

int output_appvar_palette(struct palette *palette, struct appvar *appvar)
{
    uint8_t tmp[2];
    int i;

    if (palette->include_size)
    {
        uint16_t size = palette->nr_entries * 2;

        if (validate_data_size(appvar, sizeof(uint16_t)) != 0)
        {
            return -1;
        }

        tmp[0] = size & 255;
        tmp[1] = (size >> 8) & 255;

        memcpy(&appvar->data[appvar->size], tmp, sizeof(uint16_t));
        appvar->size += 2;
    }

    for (i = 0; i < palette->nr_entries; ++i)
    {
        struct color *color = &palette->entries[i].color;

        if (validate_data_size(appvar, sizeof(uint16_t)) != 0)
        {
            return -1;
        }

        tmp[0] = color->target & 255;
        tmp[1] = (color->target >> 8) & 255;

        memcpy(&appvar->data[appvar->size], tmp, sizeof(uint16_t));
        appvar->size += 2;
    }

    return 0;
}

static void output_appvar_c_include_file_palettes(struct output *output, FILE *fdh, int *index)
{
    int i;

    for (i = 0; i < output->nr_palettes; ++i)
    {
        struct palette *palette = output->palettes[i];
        int size = palette->nr_entries * 2;

        fprintf(fdh, "#define sizeof_%s %d\n",
            palette->name,
            size);
        fprintf(fdh, "#define %s (%s_appvar[%d])\n",
            palette->name,
            output->appvar.name,
            *index);

        *index = *index + 1;
    }
}

static void output_appvar_c_include_file_converts(struct output *output, FILE *fdh, int *index)
{
    int i;

    for (i = 0; i < output->nr_converts; ++i)
    {
        int j;

        struct convert *convert = output->converts[i];
        struct tileset_group *tileset_group = convert->tileset_group;

        fprintf(fdh, "#define %s_palette_offset %d\n",
            convert->name, convert->palette_offset);

        for (j = 0; j < convert->nr_images; ++j)
        {
            struct image *image = &convert->images[j];

            fprintf(fdh, "#define %s_width %d\n",
                image->name,
                image->width);
            fprintf(fdh, "#define %s_height %d\n",
                image->name,
                image->height);

            if (image->compressed)
            {
                fprintf(fdh, "#define %s_%s_%s_compressed_index %d\n",
                    output->appvar.name,
                    convert->name,
                    image->name,
                    *index);

                fprintf(fdh, "#define %s_compressed %s_appvar[%d]\n",
                    image->name,
                    output->appvar.name,
                    *index);
            }
            else
            {
                fprintf(fdh, "#define %s_%s_%s_index %d\n",
                    output->appvar.name,
                    convert->name,
                    image->name,
                    *index);

                fprintf(fdh, "#define %s ((%s*)%s_appvar[%d])\n",
                    image->name,
                    image->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                    output->appvar.name,
                    *index);
            }

            *index = *index + 1;
        }

        if (tileset_group != NULL)
        {
            int k;

            for (k = 0; k < tileset_group->nr_tilesets; ++k)
            {
                struct tileset *tileset = &tileset_group->tilesets[k];
                int l;

                tileset->appvar_index = *index;

                fprintf(fdh, "#define %s_tile_width %d\n",
                    tileset->image.name,
                    tileset->tile_width);
                fprintf(fdh, "#define %s_tile_height %d\n",
                    tileset->image.name,
                    tileset->tile_height);

                if (tileset->compressed)
                {
                    fprintf(fdh, "#define %s_compressed %s_appvar[%d]\n",
                        tileset->image.name,
                        output->appvar.name,
                        *index);
                    fprintf(fdh, "#define %s_tiles_num %d\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                    fprintf(fdh, "extern unsigned char *%s_tiles_compressed[%d];\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        fprintf(fdh, "#define %s_tile_%d_compressed %s_tiles_compressed[%d]\n",
                        tileset->image.name,
                        l,
                        tileset->image.name,
                        l);
                    }
                }
                else
                {
                    fprintf(fdh, "#define %s %s_appvar[%d]\n",
                        tileset->image.name,
                        output->appvar.name,
                        *index);
                    fprintf(fdh, "#define %s_tiles_num %d\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                    fprintf(fdh, "extern unsigned char *%s_tiles_data[%d];\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                    fprintf(fdh, "#define %s_tiles ((%s**)%s_tiles_data)\n",
                        tileset->image.name,
                        tileset->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                        tileset->image.name);
                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        fprintf(fdh, "#define %s_tile_%d ((%s*)%s_tiles_data[%d])\n",
                        tileset->image.name,
                        l,
                        tileset->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                        tileset->image.name,
                        l);
                    }
                }

                *index = *index + 1;
            }
        }
    }
}

void output_appvar_c_include_file(struct output *output, FILE *fdh)
{
    struct appvar *appvar = &output->appvar;
    int index = 0;

    fprintf(fdh, "#ifndef %s_appvar_include_file\n", appvar->name);
    fprintf(fdh, "#define %s_appvar_include_file\n", appvar->name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");

    if (output->order == OUTPUT_PALETTES_FIRST)
    {
        output_appvar_c_include_file_palettes(output, fdh, &index);
        output_appvar_c_include_file_converts(output, fdh, &index);
    }
    else
    {
        output_appvar_c_include_file_converts(output, fdh, &index);
        output_appvar_c_include_file_palettes(output, fdh, &index);
    }

    appvar->nr_entries = index;

    fprintf(fdh, "#define %s_entries_num %d\n",
        appvar->name,
        appvar->nr_entries);

    fprintf(fdh, "extern unsigned char *%s_appvar[%d];\n",
        appvar->name,
        appvar->nr_entries);

    if (appvar->init)
    {
        if (appvar->compress != COMPRESS_NONE)
        {
            fprintf(fdh, "unsigned char %s_init(void *addr);\n",
                appvar->name);
        }
        else
        {
            fprintf(fdh, "unsigned char %s_init(void);\n",
                appvar->name);
        }
    }

    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");
}

void output_appvar_c_source_file(struct output *output, FILE *fds)
{
    struct appvar *appvar = &output->appvar;
    int offset = appvar->data_offset;
    int i, k, l;

    fprintf(fds, "#include \"%s\"\n", output->include_file);
    if (appvar->compress == COMPRESS_NONE)
    {
        fprintf(fds, "#include <fileioc.h>\n");
    }
    fprintf(fds, "\n");
    fprintf(fds, "#define %s_HEADER_SIZE %d\n",
        appvar->name, appvar->header_size);
    fprintf(fds, "\n");

    if (appvar->lut == false)
    {
        fprintf(fds, "unsigned char *%s_appvar[%d] =\n{\n",
            appvar->name,
            appvar->nr_entries);

        // global appvar mapping
        for (i = 0; i < output->nr_palettes; ++i)
        {
            struct palette *palette = output->palettes[i];

            fprintf(fds, "    (unsigned char*)%d,\n",
                offset);

            offset += palette->nr_entries * 2;
        }

        for (i = 0; i < output->nr_converts; ++i)
        {
            struct convert *convert = output->converts[i];
            struct tileset_group *tileset_group = convert->tileset_group;
            int j;

            for (j = 0; j < convert->nr_images; ++j)
            {
                fprintf(fds, "    (unsigned char*)%d,\n",
                    offset);

                offset += convert->images[j].size;
            }

            if (tileset_group != NULL)
            {
                for (k = 0; k < tileset_group->nr_tilesets; ++k)
                {
                    struct tileset *tileset = &tileset_group->tilesets[k];
                    int tileset_offset = 0;

                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        tileset_offset += tileset->tiles[l].size;
                    }

                    fprintf(fds, "    (unsigned char*)%d,\n",
                        offset);

                    offset += tileset_offset;
                }
            }
        }

        fprintf(fds, "};\n\n");
    }
    else
    {
        fprintf(fds, "unsigned char *%s_appvar[%d];\n",
            appvar->name,
            appvar->nr_entries);
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *tileset_group = convert->tileset_group;

        if (tileset_group != NULL)
        {
            for (k = 0; k < tileset_group->nr_tilesets; ++k)
            {
                struct tileset *tileset = &tileset_group->tilesets[k];

                if (appvar->lut == false)
                {
                    int tileset_offset = 0;

                    if (tileset->compressed)
                    {
                        fprintf(fds, "unsigned char *%s_tiles_compressed[%d] =\n{\n",
                            tileset->image.name,
                            tileset->nr_tiles);
                    }
                    else
                    {
                        fprintf(fds, "unsigned char *%s_tiles_data[%d] =\n{\n",
                            tileset->image.name,
                            tileset->nr_tiles);
                    }

                    for (l = 0; l < tileset->nr_tiles; l++)
                    {
                        fprintf(fds, "    (unsigned char*)%d,\n",
                            tileset_offset);

                        tileset_offset += tileset->tiles[l].size;
                    }

                    fprintf(fds, "};\n\n");
                }
                else
                {
                    if (tileset->compressed)
                    {
                        fprintf(fds, "unsigned char *%s_tiles_compressed[%d];\n",
                            tileset->image.name,
                            tileset->nr_tiles);
                    }
                    else
                    {
                        fprintf(fds, "unsigned char *%s_tiles_data[%d];\n",
                            tileset->image.name,
                            tileset->nr_tiles);
                    }
                }
            }
        }
    }

    if (appvar->init)
    {
        bool has_tilesets = false;

        for (i = 0; i < output->nr_converts; ++i)
        {
            if (output->converts[i]->tileset_group != NULL)
            {
                has_tilesets = true;
                break;
            }
        }

        if (appvar->lut == false)
        {
            if (appvar->compress != COMPRESS_NONE)
            {
                fprintf(fds, "unsigned char %s_init(void *addr)\n", appvar->name);
                fprintf(fds, "{\n");
                fprintf(fds, "    unsigned int data, i;\n\n");
                fprintf(fds, "    data = (unsigned int)addr - (unsigned int)%s_appvar[0] + %s_HEADER_SIZE;\n", appvar->name, appvar->name);
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] += data;\n", appvar->name);
                fprintf(fds, "    }\n\n");
            }
            else
            {
                fprintf(fds, "unsigned char %s_init(void)\n", appvar->name);
                fprintf(fds, "{\n");
                fprintf(fds, "    unsigned int data, i;\n");
                fprintf(fds, "    ti_var_t appvar;\n\n");
                fprintf(fds, "    appvar = ti_Open(\"%s\", \"r\");\n", appvar->name);
                fprintf(fds, "    if (appvar == 0)\n");
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    data = (unsigned int)ti_GetDataPtr(appvar) - (unsigned int)%s_appvar[0] + %s_HEADER_SIZE;\n", appvar->name, appvar->name);
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] += data;\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_Close(appvar);\n\n");
            }

            for (i = 0; i < output->nr_converts; ++i)
            {
                struct convert *convert = output->converts[i];
                struct tileset_group *tileset_group = convert->tileset_group;

                if (tileset_group != NULL)
                {
                    for (k = 0; k < tileset_group->nr_tilesets; ++k)
                    {
                        struct tileset *tileset = &tileset_group->tilesets[k];

                        if (tileset->compressed)
                        {
                            fprintf(fds, "    data = (unsigned int)%s_appvar[%d] - (unsigned int)%s_tiles_compressed[0];\n",
                                appvar->name,
                                tileset->appvar_index,
                                tileset->image.name);
                            fprintf(fds, "    for (i = 0; i < %s_tiles_num; i++)\n",
                                tileset->image.name);
                            fprintf(fds, "    {\n");
                            fprintf(fds, "        %s_tiles_compressed[i] += data;\n",
                                tileset->image.name);
                            fprintf(fds, "    }\n\n");
                        }
                        else
                        {
                            fprintf(fds, "    data = (unsigned int)%s_appvar[%d] - (unsigned int)%s_tiles_data[0];\n",
                                appvar->name,
                                tileset->appvar_index,
                                tileset->image.name);
                            fprintf(fds, "    for (i = 0; i < %s_tiles_num; i++)\n",
                                tileset->image.name);
                            fprintf(fds, "    {\n");
                            fprintf(fds, "        %s_tiles_data[i] += data;\n",
                                tileset->image.name);
                            fprintf(fds, "    }\n\n");
                        }
                    }
                }
            }

            fprintf(fds, "    return 1;\n");
            fprintf(fds, "}\n\n");
        }
        else
        {
            if (appvar->compress != COMPRESS_NONE)
            {
                fprintf(fds, "\nunsigned char %s_init(void *addr)\n", appvar->name);
                fprintf(fds, "{\n");
                if (appvar->entry_size == 3)
                {
                    fprintf(fds, "    unsigned int *table;\n");
                }
                else
                {
                    fprintf(fds, "    unsigned short *table;\n");
                }
                if (has_tilesets == true)
                {
                    fprintf(fds, "    unsigned int tileset;\n");
                }
                fprintf(fds, "    void *base;\n");
                if (appvar->total_entries < 256)
                {
                    fprintf(fds, "    unsigned char i;\n\n");
                }
                else
                {
                    fprintf(fds, "    unsigned int i;\n\n");
                }
                fprintf(fds, "    table = base = (unsigned char*)addr + %s_HEADER_SIZE;\n", appvar->name);
                fprintf(fds, "    if (*table != %d)\n", appvar->total_entries - 1);
                fprintf(fds, "    {\n");
                fprintf(fds, "        ti_Close(appvar);\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] = (void*)(*++table + (unsigned int)base);\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_Close(appvar);\n\n");
            }
            else
            {
                fprintf(fds, "\nunsigned char %s_init(void)\n", appvar->name);
                fprintf(fds, "{\n");
                fprintf(fds, "    ti_var_t appvar;\n");
                if (appvar->entry_size == 3)
                {
                    fprintf(fds, "    unsigned int *table;\n");
                }
                else
                {
                    fprintf(fds, "    unsigned short *table;\n");
                }
                fprintf(fds, "    void *base;\n");
                if (has_tilesets == true)
                {
                    fprintf(fds, "    unsigned int tileset;\n");
                }
                if (appvar->total_entries < 256)
                {
                    fprintf(fds, "    unsigned char i;\n\n");
                }
                else
                {
                    fprintf(fds, "    unsigned int i;\n\n");
                }
                fprintf(fds, "    appvar = ti_Open(\"%s\", \"r\");\n", appvar->name);
                fprintf(fds, "    if (appvar == 0)\n");
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    table = base = (char*)ti_GetDataPtr(appvar) + %s_HEADER_SIZE;\n", appvar->name);
                fprintf(fds, "    if (*table != %d)\n", appvar->total_entries - 1);
                fprintf(fds, "    {\n");
                fprintf(fds, "        ti_Close(appvar);\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] = (void*)(*++table + (unsigned int)base);\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_Close(appvar);\n\n");
            }

            for (i = 0; i < output->nr_converts; ++i)
            {
                struct convert *convert = output->converts[i];
                struct tileset_group *tileset_group = convert->tileset_group;

                if (tileset_group != NULL)
                {
                    for (k = 0; k < tileset_group->nr_tilesets; ++k)
                    {
                        struct tileset *tileset = &tileset_group->tilesets[k];

                        fprintf(fds, "    tileset = (unsigned int)%s_appvar[%d];\n",
                            appvar->name,
                            tileset->appvar_index);
                        fprintf(fds, "    for (i = 0; i < %s_tiles_num; i++)\n",
                            tileset->image.name);
                        fprintf(fds, "    {\n");

                        if (tileset->compressed)
                        {
                            fprintf(fds, "        %s_tiles_compressed[i] = (void*)(*++table + tileset);\n",
                                tileset->image.name);
                        }
                        else
                        {
                            fprintf(fds, "        %s_tiles_data[i] = (void*)(*++table + tileset);\n",
                                tileset->image.name);
                        }

                        fprintf(fds, "    }\n\n");
                    }
                }
            }

            fprintf(fds, "    return 1;\n");
            fprintf(fds, "}\n\n");
        }
    }
}

int output_appvar_include_file(struct output *output, struct appvar *appvar)
{
    char *var_name;
    char *var_c_name;
    char *tmp;
    FILE *fdh;
    FILE *fds;
    FILE *fdv;
    int ret = -1;

    if (appvar == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    var_name = strdupcat(appvar->directory, ".8xv");
    var_c_name = strdupcat(appvar->directory, ".c");
    tmp = NULL;

    if (appvar->name == NULL)
    {
        LOG_ERROR("Missing \"name\" parameter for AppVar.\n");
        goto error;
    }

    if (var_name == NULL || var_c_name == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        goto error;
    }

    switch (appvar->source)
    {
        case APPVAR_SOURCE_C:
            if (output->include_file == NULL)
            {
                LOG_ERROR("Missing \"include-file\" parameter for AppVar.\n");
                goto error;
            }

            tmp = strdupcat(output->directory, output->include_file);

            LOG_INFO(" - Writing \'%s\'\n", tmp);

            fdh = fopen(tmp, "wt");
            if (fdh == NULL)
            {
                LOG_ERROR("Could not open file: %s\n", strerror(errno));
                goto error;
            }

            output_appvar_c_include_file(output, fdh);

            LOG_INFO(" - Writing \'%s\'\n", var_c_name);

            fds = fopen(var_c_name, "wt");
            if (fds == NULL)
            {
                fclose(fdh);
                LOG_ERROR("Could not open file: %s\n", strerror(errno));
                goto error;
            }

            output_appvar_c_source_file(output, fds);

            fclose(fdh);
            fclose(fds);
            break;

        case APPVAR_SOURCE_ICE:
            break;

        case APPVAR_SOURCE_NONE:
            break;
    }

    LOG_INFO(" - Writing \'%s\'\n", var_name);

    fdv = fopen(var_name, "wb");
    if (fdv == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    ret = appvar_write(appvar, fdv);
    if (ret != 0)
    {
        fclose(fdv);
        if (remove(var_name) != 0)
        {
            LOG_ERROR("Could not remove file: %s\n", strerror(errno));
        }
        goto error;
    }

    fclose(fdv);

error:
    free(tmp);
    free(var_name);
    free(var_c_name);
    return ret;
}
