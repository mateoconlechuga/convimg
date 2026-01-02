/*
 * Copyright 2017-2025 Matt "MateoConLechuga" Waltz
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
#include "memory.h"
#include "log.h"
#include "clean.h"

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

int appvar_insert_entry(struct appvar *appvar, uint32_t index, uint32_t data)
{
    uint8_t *entry_table;
    uint8_t *entry;

    if (index >= appvar->total_entries)
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

int output_appvar_init(struct output *output)
{
    struct appvar *appvar = &output->appvar;
    uint32_t nr_entries;
    uint32_t offset;
    uint32_t index;

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

    for (uint32_t i = 0; i < output->nr_palettes; ++i)
    {
        nr_entries++;
    }

    for (uint32_t i = 0; i < output->nr_converts; ++i)
    {
        const struct convert *convert = output->converts[i];

        for (uint32_t j = 0; j < convert->nr_images; ++j)
        {
            nr_entries++;
        }

        for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
        {
            const struct tileset *tileset = &convert->tilesets[k];

            nr_entries++;

            for (uint32_t l = 0; l < tileset->nr_tiles; l++)
            {
                nr_entries++;
            }
        }
    }

    /* + 1 for lut size */
    nr_entries += 1;
    appvar->data_offset = nr_entries * appvar->entry_size;
    appvar->total_entries = nr_entries;

    /* temporary, just used to validate */
    appvar->nr_entries = appvar->total_entries;

    if (validate_data_size(appvar, appvar->data_offset) != 0)
    {
        return -1;
    }

    appvar->size += appvar->data_offset;

    /* first entry is number of entries */
    appvar_insert_entry(appvar, 0, nr_entries - 1);

    offset = appvar->data_offset;
    index = 1;

    if (output->order == OUTPUT_PALETTES_FIRST)
    {
        for (uint32_t i = 0; i < output->nr_palettes; ++i)
        {
            appvar_insert_entry(appvar, index, offset);

            offset += output->palettes[i]->nr_entries * 2;

            index++;
        }

        for (uint32_t i = 0; i < output->nr_converts; ++i)
        {
            const struct convert *convert = output->converts[i];

            for (uint32_t j = 0; j < convert->nr_images; ++j)
            {
                appvar_insert_entry(appvar, index, offset);

                offset += convert->images[j].data_size;

                index++;
            }

            for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
            {
                const struct tileset *tileset = &convert->tilesets[k];
                uint32_t tileset_offset = 0;

                appvar_insert_entry(appvar, index, offset);

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    tileset_offset += tileset->tiles[l].data_size;
                }

                offset += tileset_offset;

                index++;
            }
        }

        for (uint32_t i = 0; i < output->nr_converts; ++i)
        {
            const struct convert *convert = output->converts[i];

            for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
            {
                const struct tileset *tileset = &convert->tilesets[k];
                uint32_t tileset_offset = 0;

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    appvar_insert_entry(appvar, index, tileset_offset);

                    tileset_offset += tileset->tiles[l].data_size;

                    index++;
                }
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < output->nr_converts; ++i)
        {
            const struct convert *convert = output->converts[i];

            for (uint32_t j = 0; j < convert->nr_images; ++j)
            {
                appvar_insert_entry(appvar, index, offset);

                offset += convert->images[j].data_size;

                index++;
            }

            for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
            {
                const struct tileset *tileset = &convert->tilesets[k];
                uint32_t tileset_offset = 0;

                appvar_insert_entry(appvar, index, offset);

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    tileset_offset += tileset->tiles[l].data_size;
                }

                offset += tileset_offset;

                index++;
            }
        }

        for (uint32_t i = 0; i < output->nr_converts; ++i)
        {
            struct convert *convert = output->converts[i];

            for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
            {
                struct tileset *tileset = &convert->tilesets[k];
                uint32_t tileset_offset = 0;

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    appvar_insert_entry(appvar, index, tileset_offset);

                    tileset_offset += tileset->tiles[l].data_size;

                    index++;
                }
            }
        }

        for (uint32_t i = 0; i < output->nr_palettes; ++i)
        {
            appvar_insert_entry(appvar, index, offset);

            offset += output->palettes[i]->nr_entries * 2;

            index++;
        }
    }

    appvar->data_offset += appvar->header_size;

    return 0;
}

int output_appvar_image(struct output *output, const struct image *image)
{
    struct appvar *appvar = &output->appvar;

    if (validate_data_size(appvar, image->data_size) != 0)
    {
        return -1;
    }

    memcpy(&appvar->data[appvar->size], image->data, image->data_size);
    appvar->size += image->data_size;

    return 0;
}

int output_appvar_tileset(struct output *output, const struct tileset *tileset)
{
    struct appvar *appvar = &output->appvar;

    for (uint32_t i = 0; i < tileset->nr_tiles; ++i)
    {
        struct tileset_tile *tile = &tileset->tiles[i];

        if (validate_data_size(appvar, tile->data_size) != 0)
        {
            return -1;
        }

        memcpy(&appvar->data[appvar->size], tile->data, tile->data_size);
        appvar->size += tile->data_size;
    }

    return 0;
}

int output_appvar_palette(struct output *output, const struct palette *palette)
{
    struct appvar *appvar = &output->appvar;

    if (output->palette_sizes)
    {
        uint16_t size = palette->nr_entries * 2;

        if (validate_data_size(appvar, sizeof(uint16_t)) != 0)
        {
            return -1;
        }

        appvar->data[appvar->size] = size & 255;
        appvar->size++;
        appvar->data[appvar->size] = (size >> 8) & 255;
        appvar->size++;
    }

    for (uint32_t i = 0; i < palette->nr_entries; ++i)
    {
        uint16_t target = palette->entries[i].target;

        if (validate_data_size(appvar, sizeof(uint16_t)) != 0)
        {
            return -1;
        }

        appvar->data[appvar->size] = target & 255;
        appvar->size++;
        appvar->data[appvar->size] = (target >> 8) & 255;
        appvar->size++;
    }

    return 0;
}

static void output_appvar_c_include_file_palettes(struct output *output, FILE *fdh, uint32_t *index)
{
    for (uint32_t i = 0; i < output->nr_palettes; ++i)
    {
        struct palette *palette = output->palettes[i];
        uint32_t size = palette->nr_entries * sizeof(uint16_t);

        fprintf(fdh, "#define sizeof_%s %u\n",
            palette->name,
            size);
        fprintf(fdh, "#define %s (%s_appvar[%u])\n",
            palette->name,
            output->appvar.name,
            *index);

        *index = *index + 1;
    }
}

static void output_appvar_c_include_file_converts(struct output *output, FILE *fdh, uint32_t *index)
{
    for (uint32_t i = 0; i < output->nr_converts; ++i)
    {
        const struct convert *convert = output->converts[i];

        fprintf(fdh, "#define %s_palette_offset %u\n",
            convert->name, convert->palette_offset);

        for (uint32_t j = 0; j < convert->nr_images; ++j)
        {
            const struct image *image = &convert->images[j];

            fprintf(fdh, "#define %s_width %u\n",
                image->name,
                !image->swap_width_height ? image->width : image->height);
            fprintf(fdh, "#define %s_height %u\n",
                image->name,
                !image->swap_width_height ? image->height : image->width);

            if (image->compressed)
            {
                fprintf(fdh, "#define %s_%s_%s_compressed_index %u\n",
                    output->appvar.name,
                    convert->name,
                    image->name,
                    *index);

                fprintf(fdh, "#define %s_compressed %s_appvar[%u]\n",
                    image->name,
                    output->appvar.name,
                    *index);
            }
            else
            {
                fprintf(fdh, "#define %s_%s_%s_index %u\n",
                    output->appvar.name,
                    convert->name,
                    image->name,
                    *index);

                if (image->gfx)
                {
                    fprintf(fdh, "#define %s ((%s*)%s_appvar[%u])\n",
                        image->name,
                        image->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                        output->appvar.name,
                        *index);
                }
            }

            *index = *index + 1;
        }

        for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
        {
            struct tileset *tileset = &convert->tilesets[k];

            tileset->appvar_index = *index;

            fprintf(fdh, "#define %s_tile_width %u\n",
                tileset->image.name,
                tileset->tile_width);
            fprintf(fdh, "#define %s_tile_height %u\n",
                tileset->image.name,
                tileset->tile_height);

            if (tileset->compressed)
            {
                fprintf(fdh, "#define %s_compressed %s_appvar[%u]\n",
                    tileset->image.name,
                    output->appvar.name,
                    *index);
                fprintf(fdh, "#define %s_tiles_num %u\n",
                    tileset->image.name,
                    tileset->nr_tiles);
                fprintf(fdh, "extern unsigned char *%s_tiles_compressed[%u];\n",
                    tileset->image.name,
                    tileset->nr_tiles);

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    fprintf(fdh, "#define %s_tile_%u_compressed %s_tiles_compressed[%u]\n",
                    tileset->image.name,
                    l,
                    tileset->image.name,
                    l);
                }
            }
            else
            {
                fprintf(fdh, "#define %s %s_appvar[%u]\n",
                    tileset->image.name,
                    output->appvar.name,
                    *index);
                fprintf(fdh, "#define %s_tiles_num %u\n",
                    tileset->image.name,
                    tileset->nr_tiles);
                fprintf(fdh, "extern unsigned char *%s_tiles_data[%u];\n",
                    tileset->image.name,
                    tileset->nr_tiles);
                    
                if (tileset->image.gfx)
                {
                    fprintf(fdh, "#define %s_tiles ((%s**)%s_tiles_data)\n",
                        tileset->image.name,
                        tileset->image.rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                        tileset->image.name);

                    for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                    {
                        fprintf(fdh, "#define %s_tile_%u ((%s*)%s_tiles_data[%u])\n",
                        tileset->image.name,
                        l,
                        tileset->image.rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                        tileset->image.name,
                        l);
                    }
                }
                else
                {
                    for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                    {
                        fprintf(fdh, "#define %s_tile_%u_data %s_tiles_data[%u]\n",
                        tileset->image.name,
                        l,
                        tileset->image.name,
                        l);
                    }
                }
            }

            *index = *index + 1;
        }
    }
}

void output_appvar_c_include_file(struct output *output, FILE *fdh)
{
    struct appvar *appvar = &output->appvar;
    uint32_t index = 0;

    fprintf(fdh, "#ifndef %s_appvar_include_file\n", appvar->name);
    fprintf(fdh, "#define %s_appvar_include_file\n", appvar->name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");

    fprintf(fdh, "#define %s_appvar_size %u\n",
        appvar->name,
        (unsigned int)appvar->size);

    if (appvar->compress != COMPRESS_NONE)
    {
        fprintf(fdh, "#define %s_appvar_uncompressed_size %u\n",
            appvar->name,
            (unsigned int)appvar->uncompressed_size);
    }

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

    fprintf(fdh, "#define %s_entries_num %u\n",
        appvar->name,
        appvar->nr_entries);

    fprintf(fdh, "extern unsigned char *%s_appvar[%u];\n",
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
    uint32_t offset = appvar->data_offset;

    fprintf(fds, "#include \"%s\"\n", output->include_file);
    fprintf(fds, "#include <stdint.h>\n");
    if (appvar->compress == COMPRESS_NONE)
    {
        fprintf(fds, "#include <fileioc.h>\n");
    }
    fprintf(fds, "\n");
    fprintf(fds, "#define %s_HEADER_SIZE %u\n",
        appvar->name, appvar->header_size);
    fprintf(fds, "\n");

    if (appvar->lut == false)
    {
        fprintf(fds, "unsigned char *%s_appvar[%u] =\n{\n",
            appvar->name,
            appvar->nr_entries);

        /* global appvar mapping */
        for (uint32_t i = 0; i < output->nr_palettes; ++i)
        {
            const struct palette *palette = output->palettes[i];

            fprintf(fds, "    (unsigned char*)%u,\n",
                offset);

            offset += palette->nr_entries * 2;
        }

        for (uint32_t i = 0; i < output->nr_converts; ++i)
        {
            const struct convert *convert = output->converts[i];

            for (uint32_t j = 0; j < convert->nr_images; ++j)
            {
                fprintf(fds, "    (unsigned char*)%u,\n",
                    offset);

                offset += convert->images[j].data_size;
            }

            for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
            {
                const struct tileset *tileset = &convert->tilesets[k];
                uint32_t tileset_offset = 0;

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    tileset_offset += tileset->tiles[l].data_size;
                }

                fprintf(fds, "    (unsigned char*)%u,\n",
                    offset);

                offset += tileset_offset;
            }
        }

        fprintf(fds, "};\n\n");
    }
    else
    {
        fprintf(fds, "unsigned char *%s_appvar[%u];\n",
            appvar->name,
            appvar->nr_entries);
    }

    for (uint32_t i = 0; i < output->nr_converts; ++i)
    {
        const struct convert *convert = output->converts[i];

        for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
        {
            const struct tileset *tileset = &convert->tilesets[k];

            if (appvar->lut == false)
            {
                uint32_t tileset_offset = 0;

                if (tileset->compressed)
                {
                    fprintf(fds, "unsigned char *%s_tiles_compressed[%u] =\n{\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                }
                else
                {
                    fprintf(fds, "unsigned char *%s_tiles_data[%u] =\n{\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                }

                for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                {
                    fprintf(fds, "    (unsigned char*)%u,\n",
                        tileset_offset);

                    tileset_offset += tileset->tiles[l].data_size;
                }

                fprintf(fds, "};\n\n");
            }
            else
            {
                if (tileset->compressed)
                {
                    fprintf(fds, "unsigned char *%s_tiles_compressed[%u];\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                }
                else
                {
                    fprintf(fds, "unsigned char *%s_tiles_data[%u];\n",
                        tileset->image.name,
                        tileset->nr_tiles);
                }
            }
        }
    }

    if (appvar->init)
    {
        bool has_tilesets = false;

        for (uint32_t i = 0; i < output->nr_converts; ++i)
        {
            if (output->converts[i]->tilesets != NULL)
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
                fprintf(fds, "    uintptr_t data;\n");
                fprintf(fds, "    unsigned int i;\n\n");
                fprintf(fds, "    data = (uintptr_t)addr - (uintptr_t)%s_appvar[0] + %s_HEADER_SIZE;\n", appvar->name, appvar->name);
                fprintf(fds, "    for (i = 0; i < %u; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] += data;\n", appvar->name);
                fprintf(fds, "    }\n\n");
            }
            else
            {
                fprintf(fds, "unsigned char %s_init(void)\n", appvar->name);
                fprintf(fds, "{\n");
                fprintf(fds, "    uintptr_t data;\n");
                fprintf(fds, "    unsigned int i;\n");
                fprintf(fds, "    uint8_t appvar;\n\n");
                fprintf(fds, "    appvar = ti_Open(\"%s\", \"r\");\n", appvar->name);
                fprintf(fds, "    if (appvar == 0)\n");
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    data = (uintptr_t)ti_GetDataPtr(appvar) - (uintptr_t)%s_appvar[0] + %s_HEADER_SIZE;\n", appvar->name, appvar->name);
                fprintf(fds, "    for (i = 0; i < %u; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] += data;\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_Close(appvar);\n\n");
            }

            for (uint32_t i = 0; i < output->nr_converts; ++i)
            {
                const struct convert *convert = output->converts[i];

                for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
                {
                    const struct tileset *tileset = &convert->tilesets[k];

                    if (tileset->compressed)
                    {
                        fprintf(fds, "    data = (unsigned int)%s_appvar[%u] - (unsigned int)%s_tiles_compressed[0];\n",
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
                        fprintf(fds, "    data = (unsigned int)%s_appvar[%u] - (unsigned int)%s_tiles_data[0];\n",
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
                fprintf(fds, "    if (*table != %u)\n", appvar->total_entries - 1);
                fprintf(fds, "    {\n");
                fprintf(fds, "        ti_Close(appvar);\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    for (i = 0; i < %u; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] = (void*)(*++table + (unsigned int)base);\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_Close(appvar);\n\n");
            }
            else
            {
                fprintf(fds, "\nunsigned char %s_init(void)\n", appvar->name);
                fprintf(fds, "{\n");
                fprintf(fds, "    uint8_t appvar;\n");
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
                fprintf(fds, "    if (*table != %u)\n", appvar->total_entries - 1);
                fprintf(fds, "    {\n");
                fprintf(fds, "        ti_Close(appvar);\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    for (i = 0; i < %u; i++)\n", appvar->nr_entries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] = (void*)(*++table + (unsigned int)base);\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_Close(appvar);\n\n");
            }

            for (uint32_t i = 0; i < output->nr_converts; ++i)
            {
                const struct convert *convert = output->converts[i];

                for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
                {
                    const struct tileset *tileset = &convert->tilesets[k];

                    fprintf(fds, "    tileset = (unsigned int)%s_appvar[%u];\n",
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

            fprintf(fds, "    return 1;\n");
            fprintf(fds, "}\n\n");
        }
    }
}

void output_appvar_asm_include_file(struct output *output, FILE *fdh)
{
    struct appvar *appvar = &output->appvar;
    uint32_t offset = appvar->data_offset;
    uint32_t nr_entries = 0;
    bool order = output->order;

    fprintf(fdh, "%s_appvar_size := %u\n",
        appvar->name,
        (unsigned int)appvar->size);

    if (appvar->compress != COMPRESS_NONE)
    {
        fprintf(fdh, "%s_appvar_uncompressed_size := %u\n",
            appvar->name,
            (unsigned int)appvar->uncompressed_size);
    }

    for (uint32_t o = 0; o < 2; ++o)
    {
        if (order == OUTPUT_PALETTES_FIRST)
        {
            for (uint32_t i = 0; i < output->nr_palettes; ++i)
            {
                const struct palette *palette = output->palettes[i];
                const uint32_t size = palette->nr_entries * sizeof(uint16_t);

                fprintf(fdh, "%s_%s_size := %u\n",
                    output->appvar.name,
                    palette->name,
                    size);

                fprintf(fdh, "%s_%s_offset := %u\n",
                    output->appvar.name,
                    palette->name,
                    offset);

                nr_entries++;

                offset += size;
            }
        }
        else
        {
            for (uint32_t i = 0; i < output->nr_converts; ++i)
            {
                const struct convert *convert = output->converts[i];

                fprintf(fdh, "%s_%s_palette_offset := %u\n",
                    output->appvar.name,
                    convert->name,
                    convert->palette_offset);

                for (uint32_t j = 0; j < convert->nr_images; ++j)
                {
                    const struct image *image = &convert->images[j];

                    fprintf(fdh, "%s_%s_%s_width := %u\n",
                        output->appvar.name,
                        convert->name,
                        image->name,
                        !image->swap_width_height ? image->width : image->height);
                    fprintf(fdh, "%s_%s_%s_height := %u\n",
                        output->appvar.name,
                        convert->name,
                        image->name,
                        !image->swap_width_height ? image->height : image->width);

                    if (image->compressed)
                    {
                        fprintf(fdh, "%s_%s_%s_compressed_offset := %u\n",
                            output->appvar.name,
                            convert->name,
                            image->name,
                            offset);
                    }
                    else
                    {
                        fprintf(fdh, "%s_%s_%s_offset := %u\n",
                            output->appvar.name,
                            convert->name,
                            image->name,
                            offset);
                    }

                    nr_entries++;

                    offset += convert->images[j].data_size;
                }

                for (uint32_t k = 0; k < convert->nr_tilesets; ++k)
                {
                    struct tileset *tileset = &convert->tilesets[k];
                    uint32_t tileset_offset = 0;

                    tileset->appvar_index = nr_entries;

                    fprintf(fdh, "%s_%s_%s_tile_width := %u\n",
                        output->appvar.name,
                        convert->name,
                        tileset->image.name,
                        tileset->tile_width);
                    fprintf(fdh, "%s_%s_%s_tile_height := %u\n",
                        output->appvar.name,
                        convert->name,
                        tileset->image.name,
                        tileset->tile_height);
                    fprintf(fdh, "%s_%s_%s_tiles_num := %u\n",
                        output->appvar.name,
                        convert->name,
                        tileset->image.name,
                        tileset->nr_tiles);
                    fprintf(fdh, "%s_%s_%s_tiles%soffset := %u\n",
                        output->appvar.name,
                        convert->name,
                        tileset->image.name,
                        tileset->compressed ? "_compressed_" : "_",
                        offset);

                    for (uint32_t l = 0; l < tileset->nr_tiles; l++)
                    {
                        tileset_offset += tileset->tiles[l].data_size;

                        fprintf(fdh, "%s_%s_%s_tile_%u%soffset := %u\n",
                            output->appvar.name,
                            convert->name,
                            tileset->image.name,
                            l,
                            tileset->compressed ? "_compressed_" : "_",
                            offset + tileset_offset);
                    }

                    nr_entries++;

                    offset += tileset_offset;
                }
            }
        }

        order = order == OUTPUT_PALETTES_FIRST ?
            OUTPUT_CONVERTS_FIRST : OUTPUT_PALETTES_FIRST;

        fprintf(fdh, "\n");
    }

    appvar->nr_entries = nr_entries;

    fprintf(fdh, "%s_entries_num := %u\n",
        appvar->name,
        appvar->nr_entries);

    fprintf(fdh, "%s_header_size := %u\n",
        appvar->name,
        appvar->header_size);
}

int output_appvar_include(struct output *output)
{
    struct appvar *appvar = &output->appvar;
    char *var_name = NULL;
    char *var_c_name = NULL;
    FILE *fdh = NULL;
    FILE *fds = NULL;

    var_name = strings_concat(output->directory, appvar->name, ".8xv", 0);
    if (var_name == NULL)
    {
        goto error;
    }

    var_c_name = strings_concat(output->directory, appvar->name, ".c", 0);
    if (var_c_name == NULL)
    {
        goto error;
    }

    appvar->uncompressed_size = appvar->size;

    if (appvar->compress != COMPRESS_NONE)
    {
        size_t size = appvar->size;
        void *original_data = appvar->data;

        LOG_INFO(" - Compressing AppVar \'%s\'\n", appvar->name);

        appvar->data = compress_array(original_data, &size, appvar->compress);
        free(original_data);

        if (appvar->data == NULL)
        {
            LOG_ERROR("Failed to compress AppVar.\n");
            goto error;
        }

        appvar->size = size;
    }

    if (appvar->size > APPVAR_MAX_DATA_SIZE)
    {
        LOG_ERROR("Too much data for AppVar \'%s\'.\n", appvar->name);
        goto error;
    }

    switch (appvar->source)
    {
        case APPVAR_SOURCE_C:
        {
            LOG_INFO(" - Writing \'%s\'\n", output->include_file);

            fdh = clean_fopen(output->include_file, "wt");
            if (fdh == NULL)
            {
                LOG_ERROR("Could not open file: %s\n", strerror(errno));
                goto error;
            }

            output_appvar_c_include_file(output, fdh);

            fclose(fdh);

            LOG_INFO(" - Writing \'%s\'\n", var_c_name);

            fds = clean_fopen(var_c_name, "wt");
            if (fds == NULL)
            {
                LOG_ERROR("Could not open file: %s\n", strerror(errno));
                goto error;
            }

            output_appvar_c_source_file(output, fds);

            fclose(fds);
            break;
        }

        case APPVAR_SOURCE_ASM:
        {
            LOG_INFO(" - Writing \'%s\'\n", output->include_file);

            fdh = clean_fopen(output->include_file, "wt");
            if (fdh == NULL)
            {
                LOG_ERROR("Could not open file: %s\n", strerror(errno));
                goto error;
            }

            output_appvar_asm_include_file(output, fdh);

            fclose(fdh);
            break;
        }

        default:
            break;
    }

    if (appvar_write(appvar, var_name))
    {
        LOG_ERROR("Failed to write AppVar.\n");
        goto error;
    }

    free(var_name);
    free(var_c_name);

    return 0;

error:
    free(var_name);
    free(var_c_name);
    return -1;
}
