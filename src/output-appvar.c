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

#include "output.h"
#include "appvar.h"
#include "strings.h"
#include "log.h"

#include <string.h>
#include <errno.h>

/*
 * Validates the added size fits.
 */
static int validate_data_size(appvar_t *appvar, int adding)
{
    if (appvar->size + adding >= APPVAR_MAX_BEFORE_COMPRESSION_SIZE)
    {
        LL_ERROR("Too much data for AppVar \'%s\'.", appvar->name);
        return 1;
    }

    return 0;
}

/*
 * Insert an entry into the appvar lut.
 */
int appvar_insert_entry(appvar_t *appvar, int index, int data)
{
    uint8_t *entryTable = &appvar->data[appvar->headerSize];
    uint8_t *entry;

    if (index > appvar->totalEntries - 1)
    {
        LL_ERROR("Invalid index for LUT entry.");
        return 1;
    }

    entryTable = &appvar->data[appvar->headerSize];
    entry = entryTable + index * appvar->entrySize;

    if (appvar->entrySize == 2)
    {
        entry[0] = data & 0xff;
        entry[1] = data >> 8 & 0xff;
    }
    else if (appvar->entrySize == 3)
    {
        entry[0] = data & 0xff;
        entry[1] = data >> 8 & 0xff;
        entry[2] = data >> 16 & 0xff;
    }
    else
    {
        return 1;
    }

    return 0;
}


/*
 * Outputs an AppVar header.
 */
int output_appvar_header(output_t *output, appvar_t *appvar)
{
    int numEntries;
    int i, j, k, l;
    int offset;
    int index;

    if (appvar->headerSize > 0)
    {
        if (validate_data_size(appvar, appvar->headerSize) != 0)
        {
            return 1;
        }

        memcpy(&appvar->data[appvar->size], appvar->header, appvar->headerSize);
        appvar->size += appvar->headerSize;
    }

    if (appvar->lut == false)
    {
        appvar->dataOffset = appvar->headerSize;
        return 0;
    }

    numEntries = 0;

    /* count number of palettes */
    for (i = 0; i < output->numPalettes; ++i)
    {
        numEntries++;
    }

    /* count number of images + tilesets */
    for (i = 0; i < output->numConverts; ++i)
    {
        convert_t *convert = output->converts[i];
        tileset_group_t *tilesetGroup = convert->tilesetGroup;

        for (j = 0; j < convert->numImages; ++j)
        {
            numEntries++;
        }

        if (tilesetGroup != NULL)
        {
            for (k = 0; k < tilesetGroup->numTilesets; ++k)
            {
                tileset_t *tileset = &tilesetGroup->tilesets[k];

                numEntries++;

                for (l = 0; l < tileset->numTiles; l++)
                {
                    numEntries++;
                }
            }
        }
    }

    /* + 1 for lut size */
    numEntries += 1;
    appvar->dataOffset = numEntries * appvar->entrySize;
    appvar->totalEntries = numEntries;

    /* temp, just used to validate */
    appvar->numEntries = appvar->totalEntries;

    if (validate_data_size(appvar, appvar->dataOffset) != 0)
    {
        return 1;
    }

    appvar->size += appvar->dataOffset;
    
    /* first entry is number of entries */
    appvar_insert_entry(appvar, 0, numEntries - 1);

    offset = appvar->dataOffset;
    index = 1;

    if (output->order == OUTPUT_PALETTES_FIRST)
    {
        /* output palette entries */
        for (i = 0; i < output->numPalettes; ++i)
        {
            appvar_insert_entry(appvar, index, offset);

            offset += output->palettes[i]->numEntries * 2;

            index++;
        }

        /* output entries for images + tilesets */
        for (i = 0; i < output->numConverts; ++i)
        {
            convert_t *convert = output->converts[i];
            tileset_group_t *tilesetGroup = convert->tilesetGroup;

            for (j = 0; j < convert->numImages; ++j)
            {
                appvar_insert_entry(appvar, index, offset);

                offset += convert->images[j].size;

                index++;
            }

            if (tilesetGroup != NULL)
            {
                for (k = 0; k < tilesetGroup->numTilesets; ++k)
                {
                    tileset_t *tileset = &tilesetGroup->tilesets[k];
                    int tilesetOffset = 0;

                    appvar_insert_entry(appvar, index, offset);

                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        tilesetOffset += tileset->tiles[l].size;
                    }

                    offset += tilesetOffset;

                    index++;
                }
            }
        }
        for (i = 0; i < output->numConverts; ++i)
        {
            convert_t *convert = output->converts[i];
            tileset_group_t *tilesetGroup = convert->tilesetGroup;

            if (tilesetGroup != NULL)
            {
                for (k = 0; k < tilesetGroup->numTilesets; ++k)
                {
                    tileset_t *tileset = &tilesetGroup->tilesets[k];
                    int tilesetOffset = 0;

                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        appvar_insert_entry(appvar, index, tilesetOffset);

                        tilesetOffset += tileset->tiles[l].size;

                        index++;
                    }
                }
            }
        }
    }
    else
    {
        /* output entries for images + tilesets */
        for (i = 0; i < output->numConverts; ++i)
        {
            convert_t *convert = output->converts[i];
            tileset_group_t *tilesetGroup = convert->tilesetGroup;

            for (j = 0; j < convert->numImages; ++j)
            {
                appvar_insert_entry(appvar, index, offset);

                offset += convert->images[j].size;

                index++;
            }

            if (tilesetGroup != NULL)
            {
                for (k = 0; k < tilesetGroup->numTilesets; ++k)
                {
                    tileset_t *tileset = &tilesetGroup->tilesets[k];
                    int tilesetOffset = 0;

                    appvar_insert_entry(appvar, index, offset);

                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        tilesetOffset += tileset->tiles[l].size;
                    }

                    offset += tilesetOffset;

                    index++;
                }
            }
        }
        for (i = 0; i < output->numConverts; ++i)
        {
            convert_t *convert = output->converts[i];
            tileset_group_t *tilesetGroup = convert->tilesetGroup;

            if (tilesetGroup != NULL)
            {
                for (k = 0; k < tilesetGroup->numTilesets; ++k)
                {
                    tileset_t *tileset = &tilesetGroup->tilesets[k];
                    int tilesetOffset = 0;

                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        appvar_insert_entry(appvar, index, tilesetOffset);

                        tilesetOffset += tileset->tiles[l].size;

                        index++;
                    }
                }
            }
        }

        /* output palette entries */
        for (i = 0; i < output->numPalettes; ++i)
        {
            appvar_insert_entry(appvar, index, offset);

            offset += output->palettes[i]->numEntries * 2;

            index++;
        }
    }

    appvar->dataOffset += appvar->headerSize;

    return 0;
}


/*
 * Outputs a converted AppVar image.
 */
int output_appvar_image(image_t *image, appvar_t *appvar)
{
    if (validate_data_size(appvar, image->size) != 0)
    {
        return 1;
    }

    memcpy(&appvar->data[appvar->size], image->data, image->size);
    appvar->size += image->size;

    return 0;
}

/*
 * Outputs a converted AppVar tileset.
 */
int output_appvar_tileset(tileset_t *tileset, appvar_t *appvar)
{
    int i;

    for (i = 0; i < tileset->numTiles; ++i)
    {
        tileset_tile_t *tile = &tileset->tiles[i];

        if (validate_data_size(appvar, tile->size) != 0)
        {
            return 1;
        }

        memcpy(&appvar->data[appvar->size], tile->data, tile->size);
        appvar->size += tile->size;
    }

    return 0;
}

/*
 * Outputs a converted AppVar palette.
 */
int output_appvar_palette(palette_t *palette, appvar_t *appvar)
{
    uint8_t tmp[2];
    int i;

    if (palette->includeSize)
    {
        uint16_t size = palette->numEntries * 2;

        if (validate_data_size(appvar, sizeof(uint16_t)) != 0)
        {
            return 1;
        }

        tmp[0] = size & 255;
        tmp[1] = (size >> 8) & 255;

        memcpy(&appvar->data[appvar->size], tmp, sizeof(uint16_t));
        appvar->size += 2;
    }

    for (i = 0; i < palette->numEntries; ++i)
    {
        color_t *color = &palette->entries[i].color;

        if (validate_data_size(appvar, sizeof(uint16_t)) != 0)
        {
            return 1;
        }

        tmp[0] = color->target & 255;
        tmp[1] = (color->target >> 8) & 255;

        memcpy(&appvar->data[appvar->size], tmp, sizeof(uint16_t));
        appvar->size += 2;
    }

    return 0;
}

static void output_appvar_c_include_file_palettes(output_t *output, FILE *fdh, int *index)
{
    int i;

    for (i = 0; i < output->numPalettes; ++i)
    {
        palette_t *palette = output->palettes[i];
        int size = palette->numEntries * 2;

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

static void output_appvar_c_include_file_converts(output_t *output, FILE *fdh, int *index)
{
    int i;

    for (i = 0; i < output->numConverts; ++i)
    {
        int j;

        convert_t *convert = output->converts[i];
        tileset_group_t *tilesetGroup = convert->tilesetGroup;

        fprintf(fdh, "#define %s_palette_offset %d\n",
            convert->name, convert->paletteOffset);

        for (j = 0; j < convert->numImages; ++j)
        {
            image_t *image = &convert->images[j];

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

                fprintf(fdh, "#define %s ((gfx_sprite_t*)%s_appvar[%d])\n",
                    image->name,
                    output->appvar.name,
                    *index);
            }

            *index = *index + 1;
        }

        if (tilesetGroup != NULL)
        {
            int k;

            for (k = 0; k < tilesetGroup->numTilesets; ++k)
            {
                tileset_t *tileset = &tilesetGroup->tilesets[k];
                int l;

                tileset->appvarIndex = *index;

                fprintf(fdh, "#define %s_tile_width %d\n",
                    tileset->image.name,
                    tileset->tileWidth);
                fprintf(fdh, "#define %s_tile_height %d\n",
                    tileset->image.name,
                    tileset->tileHeight);

                if (tileset->compressed)
                {
                    fprintf(fdh, "#define %s_compressed %s_appvar[%d]\n",
                        tileset->image.name,
                        output->appvar.name,
                        *index);
                    fprintf(fdh, "#define %s_tiles_num %d\n",
                        tileset->image.name,
                        tileset->numTiles);
                    fprintf(fdh, "extern unsigned char *%s_tiles_compressed[%d];\n",
                        tileset->image.name,
                        tileset->numTiles);
                    for (l = 0; l < tileset->numTiles; l++)
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
                        tileset->numTiles);
                    fprintf(fdh, "extern unsigned char *%s_tiles_data[%d];\n",
                        tileset->image.name,
                        tileset->numTiles);
                    fprintf(fdh, "#define %s_tiles ((gfx_sprite_t**)%s_tiles_data)\n",
                        tileset->image.name,
                        tileset->image.name);
                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        fprintf(fdh, "#define %s_tile_%d ((gfx_sprite_t*)%s_tiles_data[%d])\n",
                        tileset->image.name,
                        l,
                        tileset->image.name,
                        l);
                    }
                }

                *index = *index + 1;
            }
        }
    }
}

/*
 * Outputs a C style header.
 */
void output_appvar_c_include_file(output_t *output, FILE *fdh)
{
    appvar_t *appvar = &output->appvar;
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

    appvar->numEntries = index;

    fprintf(fdh, "#define %s_entries_num %d\n",
        appvar->name,
        appvar->numEntries);

    fprintf(fdh, "extern unsigned char *%s_appvar[%d];\n",
        appvar->name,
        appvar->numEntries);

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

/*
 * Outputs a C style source file.
 */
void output_appvar_c_source_file(output_t *output, FILE *fds)
{
    appvar_t *appvar = &output->appvar;
    int offset = appvar->dataOffset;
    int i, j, k, l;

    fprintf(fds, "#include \"%s\"\n", output->includeFileName);
    if (appvar->compress == COMPRESS_NONE)
    {
        fprintf(fds, "#include <fileioc.h>\n");
    }
    fprintf(fds, "\n");
    fprintf(fds, "#define %s_HEADER_SIZE %u\n",
        appvar->name, appvar->headerSize);
    fprintf(fds, "\n");

    if (appvar->lut == false)
    {
        fprintf(fds, "unsigned char *%s_appvar[%d] =\n{\n",
            appvar->name,
            appvar->numEntries);

        /* output global appvar mapping */
        for (i = 0; i < output->numPalettes; ++i)
        {
            palette_t *palette = output->palettes[i];

            fprintf(fds, "    (unsigned char*)%d,\n",
                offset);

            offset += palette->numEntries * 2;
        }

        for (i = 0; i < output->numConverts; ++i)
        {
            convert_t *convert = output->converts[i];
            tileset_group_t *tilesetGroup = convert->tilesetGroup;

            for (j = 0; j < convert->numImages; ++j)
            {
                fprintf(fds, "    (unsigned char*)%d,\n",
                    offset);

                offset += convert->images[j].size;
            }

            if (tilesetGroup != NULL)
            {
                for (k = 0; k < tilesetGroup->numTilesets; ++k)
                {
                    tileset_t *tileset = &tilesetGroup->tilesets[k];
                    int tilesetOffset = 0;

                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        tilesetOffset += tileset->tiles[l].size;
                    }

                    fprintf(fds, "    (unsigned char*)%d,\n",
                        offset);

                    offset += tilesetOffset;
                }
            }
        }

        fprintf(fds, "};\n\n");
    }
    else
    {
        fprintf(fds, "unsigned char *%s_appvar[%d];\n",
            appvar->name,
            appvar->numEntries);
    }

    /* output tilemap tables */
    for (i = 0; i < output->numConverts; ++i)
    {
        convert_t *convert = output->converts[i];
        tileset_group_t *tilesetGroup = convert->tilesetGroup;

        if (tilesetGroup != NULL)
        {
            for (k = 0; k < tilesetGroup->numTilesets; ++k)
            {
                tileset_t *tileset = &tilesetGroup->tilesets[k];
                int tilesetOffset = 0;

                if (appvar->lut == false)
                {
                    if (tileset->compressed)
                    {
                        fprintf(fds, "unsigned char *%s_tiles_compressed[%d] =\n{\n",
                            tileset->image.name,
                            tileset->numTiles);
                    }
                    else
                    {
                        fprintf(fds, "unsigned char *%s_tiles_data[%d] =\n{\n",
                            tileset->image.name,
                            tileset->numTiles);
                    }

                    for (l = 0; l < tileset->numTiles; l++)
                    {
                        fprintf(fds, "    (unsigned char*)%d,\n",
                            tilesetOffset);

                        tilesetOffset += tileset->tiles[l].size;
                    }

                    fprintf(fds, "};\n\n");
                }
                else
                {
                    if (tileset->compressed)
                    {
                        fprintf(fds, "unsigned char *%s_tiles_compressed[%d];\n",
                            tileset->image.name,
                            tileset->numTiles);
                    }
                    else
                    {
                        fprintf(fds, "unsigned char *%s_tiles_data[%d];\n",
                            tileset->image.name,
                            tileset->numTiles);
                    }
                }
            }
        }
    }

    if (appvar->init)
    {
        bool hasTilesets = false;

        for (i = 0; i < output->numConverts; ++i)
        {
            if (output->converts[i]->tilesetGroup != NULL)
            {
                hasTilesets = true;
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
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->numEntries);
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
                fprintf(fds, "    ti_CloseAll();\n\n");
                fprintf(fds, "    appvar = ti_Open(\"%s\", \"r\");\n", appvar->name);
                fprintf(fds, "    if (appvar == 0)\n");
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    data = (unsigned int)ti_GetDataPtr(appvar) - (unsigned int)%s_appvar[0] + %s_HEADER_SIZE;\n", appvar->name, appvar->name);
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->numEntries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] += data;\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_CloseAll();\n\n");
            }

            /* output tilemap init */
            for (i = 0; i < output->numConverts; ++i)
            {
                convert_t *convert = output->converts[i];
                tileset_group_t *tilesetGroup = convert->tilesetGroup;

                if (tilesetGroup != NULL)
                {
                    for (k = 0; k < tilesetGroup->numTilesets; ++k)
                    {
                        tileset_t *tileset = &tilesetGroup->tilesets[k];

                        if (tileset->compressed)
                        {
                            fprintf(fds, "    data = (unsigned int)%s_appvar[%u] - (unsigned int)%s_tiles_compressed[0];\n",
                                appvar->name,
                                tileset->appvarIndex,
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
                                tileset->appvarIndex,
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
                if (appvar->entrySize == 3)
                {
                    fprintf(fds, "    unsigned int *table;\n");
                }
                else
                {
                    fprintf(fds, "    unsigned short *table;\n");
                }
                if (hasTilesets == true)
                {
                    fprintf(fds, "    unsigned int tileset;\n");
                }
                fprintf(fds, "    void *base;\n");
                if (appvar->totalEntries < 256)
                {
                    fprintf(fds, "    unsigned char i;\n\n");
                }
                else
                {
                    fprintf(fds, "    unsigned int i;\n\n");
                }
                fprintf(fds, "    table = base = (unsigned char*)addr + %s_HEADER_SIZE;\n", appvar->name);
                fprintf(fds, "    if (*table != %d)\n", appvar->totalEntries - 1);
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->numEntries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] = (void*)(*++table + (unsigned int)base);\n", appvar->name);
                fprintf(fds, "    }\n\n");
            }
            else
            {
                fprintf(fds, "\nunsigned char %s_init(void)\n", appvar->name);
                fprintf(fds, "{\n");
                fprintf(fds, "    ti_var_t appvar;\n");
                if (appvar->entrySize == 3)
                {
                    fprintf(fds, "    unsigned int *table;\n");
                }
                else
                {
                    fprintf(fds, "    unsigned short *table;\n");
                }
                fprintf(fds, "    void *base;\n");
                if (hasTilesets == true)
                {
                    fprintf(fds, "    unsigned int tileset;\n");
                }
                if (appvar->totalEntries < 256)
                {
                    fprintf(fds, "    unsigned char i;\n\n");
                }
                else
                {
                    fprintf(fds, "    unsigned int i;\n\n");
                }
                fprintf(fds, "    ti_CloseAll();\n\n");
                fprintf(fds, "    appvar = ti_Open(\"%s\", \"r\");\n", appvar->name);
                fprintf(fds, "    if (appvar == 0)\n");
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    table = base = (char*)ti_GetDataPtr(appvar) + %s_HEADER_SIZE;\n", appvar->name);
                fprintf(fds, "    if (*table != %d)\n", appvar->totalEntries - 1);
                fprintf(fds, "    {\n");
                fprintf(fds, "        return 0;\n");
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    for (i = 0; i < %d; i++)\n", appvar->numEntries);
                fprintf(fds, "    {\n");
                fprintf(fds, "        %s_appvar[i] = (void*)(*++table + (unsigned int)base);\n", appvar->name);
                fprintf(fds, "    }\n\n");
                fprintf(fds, "    ti_CloseAll();\n\n");
            }

            /* output tilemap init */
            for (i = 0; i < output->numConverts; ++i)
            {
                convert_t *convert = output->converts[i];
                tileset_group_t *tilesetGroup = convert->tilesetGroup;

                if (tilesetGroup != NULL)
                {
                    for (k = 0; k < tilesetGroup->numTilesets; ++k)
                    {
                        tileset_t *tileset = &tilesetGroup->tilesets[k];

                        fprintf(fds, "    tileset = (unsigned int)%s_appvar[%u];\n",
                            appvar->name,
                            tileset->appvarIndex);
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

/*
 * Outputs an include file for the output structure
 */
int output_appvar_include_file(output_t *output, appvar_t *appvar)
{
    char *varName = strdupcat(appvar->directory, ".8xv");
    char *varCName = strdupcat(appvar->directory, ".c");
    char *tmp;
    FILE *fdh;
    FILE *fds;
    FILE *fdv;
    int ret = 1;

    if (appvar == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        goto error;
    }

    if (appvar->name == NULL)
    {
        LL_ERROR("Missing \"name\" parameter for AppVar.");
        goto error;
    }

    if (varName == NULL || varCName == NULL)
    {
        LL_DEBUG("Memory error in %s", __func__);
        goto error;
    }

    switch (appvar->source)
    {
        case APPVAR_SOURCE_C:
            if (output->includeFileName == NULL)
            {
                LL_ERROR("Missing \"include-file\" parameter for AppVar.");
                goto error;
            }

            tmp = strdupcat(output->directory, output->includeFileName);

            LL_INFO(" - Writing \'%s\'", tmp);

            fdh = fopen(tmp, "wt");
            if (fdh == NULL)
            {
                LL_ERROR("Could not open file: %s", strerror(errno));
                goto error;
            }

            output_appvar_c_include_file(output, fdh);

            LL_INFO(" - Writing \'%s\'", varCName);

            fds = fopen(varCName, "wt");
            if (fds == NULL)
            {
                fclose(fdh);
                LL_ERROR("Could not open file: %s", strerror(errno));
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

    LL_INFO(" - Writing \'%s\'", varName);

    fdv = fopen(varName, "wb");
    if (fdv == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    ret = appvar_write(appvar, fdv);
    if (ret != 0)
    {
        fclose(fdv);
        remove(varName);
        goto error;
    }

    fclose(fdv);

error:
    free(varName);
    free(varCName);
    return ret;
}
