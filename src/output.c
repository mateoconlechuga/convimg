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
#include "output-formats.h"
#include "strings.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

/*
 * Allocates an output structure.
 */
output_t *output_alloc(void)
{
    output_t *output = NULL;

    output = malloc(sizeof(output_t));
    if (output == NULL)
    {
        return NULL;
    }

    output->name = NULL;
    output->includeFileName = NULL;
    output->directory = strdup("");
    output->convertNames = NULL;
    output->numConverts = 0;
    output->converts = NULL;
    output->paletteNames = NULL;
    output->palettes = NULL;
    output->numPalettes = 0;
    output->format = OUTPUT_FORMAT_INVALID;
    output->appvar.name = NULL;
    output->appvar.directory = NULL;
    output->appvar.archived = true;
    output->appvar.init = true;
    output->appvar.source = APPVAR_SOURCE_C;
    output->appvar.compress = COMPRESS_NONE;
    output->appvar.data = malloc(APPVAR_MAX_DATA_SIZE);
    output->appvar.size = 0;
    output->appvar.header = NULL;
    output->appvar.header_size = 0;

    return output;
}

/*
 * Adds a new convert to the output structure.
 */
int output_add_convert(output_t *output, const char *convertName)
{
    if (output == NULL ||
        convertName == NULL ||
        output->format == OUTPUT_FORMAT_INVALID)
    {
        LL_DEBUG("Invalid param in %s'.", __func__);
        return 1;
    }

    output->convertNames =
        realloc(output->convertNames, (output->numConverts + 1) * sizeof(char *));
    if (output->convertNames == NULL)
    {
        LL_ERROR("Memory error in %s'.", __func__);
        return 1;
    }

    output->convertNames[output->numConverts] = strdup(convertName);
    output->numConverts++;

    return 0;
}

/*
 * Adds a new palette to the output structure.
 */
int output_add_palette(output_t *output, const char *paletteName)
{
    if (output == NULL ||
        paletteName == NULL ||
        output->format == OUTPUT_FORMAT_INVALID)
    {
        LL_DEBUG("Invalid param in %s'.", __func__);
        return 1;
    }

    output->paletteNames =
        realloc(output->paletteNames, (output->numPalettes + 1) * sizeof(char *));
    if (output->paletteNames == NULL)
    {
        LL_ERROR("Memory error in %s'.", __func__);
        return 1;
    }

    output->paletteNames[output->numPalettes] = strdup(paletteName);
    output->numPalettes++;

    return 0;
}

/*
 * Frees an allocated output structure.
 */
void output_free(output_t *output)
{
    int i;

    if (output == NULL)
    {
        return;
    }

    for (i = 0; i < output->numConverts; ++i)
    {
        free(output->convertNames[i]);
        output->convertNames[i] = NULL;
    }

    for (i = 0; i < output->numPalettes; ++i)
    {
        free(output->paletteNames[i]);
        output->paletteNames[i] = NULL;
    }

    free(output->convertNames);
    output->convertNames = NULL;

    free(output->paletteNames);
    output->paletteNames = NULL;

    free(output->appvar.name);
    output->appvar.name = NULL;

    free(output->appvar.header);
    output->appvar.name = NULL;

    free(output->appvar.data);
    output->appvar.data = NULL;

    free(output->converts);
    output->converts = NULL;

    free(output->palettes);
    output->palettes = NULL;

    free(output->includeFileName);
    output->includeFileName = NULL;

    free(output->name);
    output->name = NULL;

    free(output->directory);
    output->directory = NULL;

    output->numConverts = 0;
    output->numPalettes = 0;
}

/*
 * Sets up paths for output.
 */
int output_init(output_t *output)
{
    char *tmp;

    tmp = output->includeFileName;
    output->includeFileName =
        strdupcat(output->directory, output->includeFileName);
    free(tmp);

    if (output->format == OUTPUT_FORMAT_ICE)
    {
        remove(output->includeFileName);
    }

    if (output->format == OUTPUT_FORMAT_APPVAR)
    {
        output_appvar_header(&output->appvar);
    }

    if (output->appvar.name != NULL)
    {
        output->appvar.directory =
            strdupcat(output->directory, output->appvar.name);
    }

    return 0;
}

/*
 * Find the convert containing the data to output.
 */
static int output_find_converts(output_t *output, convert_t **converts, int numConverts)
{
    int i, j;

    if (output == NULL || converts == NULL)
    {
        LL_DEBUG("Invalid param in %s.", __func__);
        return 1;
    }

    output->converts = malloc(output->numConverts * sizeof(convert_t *));
    if (output->converts == NULL)
    {
        LL_DEBUG("Memory error in %s.", __func__);
        return 1;
    }

    for (i = 0; i < output->numConverts; ++i)
    {
        for (j = 0; j < numConverts; ++j)
        {
            if (!strcmp(output->convertNames[i], converts[j]->name))
            {
                output->converts[i] = converts[j];
                goto nextconvert;
            }
        }

        LL_ERROR("No matching convert name \'%s\' found for output.",
                 output->convertNames[i]);
        return 1;

nextconvert:
        continue;
    }

    return 0;
}

/*
 * Find the palettes containing the data to output.
 */
static int output_find_palettes(output_t *output, palette_t **palettes, int numPalettes)
{
    int i, j;

    if (output == NULL || palettes == NULL)
    {
        LL_DEBUG("Invalid param in %s.", __func__);
        return 1;
    }

    output->palettes = malloc(output->numPalettes * sizeof(palette_t *));
    if (output->palettes == NULL)
    {
        LL_DEBUG("Memory error in %s.", __func__);
        return 1;
    }

    for (i = 0; i < output->numPalettes; ++i)
    {
        for (j = 0; j < numPalettes; ++j)
        {
            if (!strcmp(output->paletteNames[i], palettes[j]->name))
            {
                output->palettes[i] = palettes[j];
                goto nextpalette;
            }
        }

        LL_ERROR("No matching convert name \'%s\' found for output.",
                 output->paletteNames[i]);
        return 1;

nextpalette:
        continue;
    }

    return 0;
}

/*
 * Output converted images into the desired format.
 */
int output_converts(output_t *output, convert_t **converts, int numConverts)
{
    int ret = 0;
    int i;

    if (numConverts == 0)
    {
        return 0;
    }

    ret = output_find_converts(output, converts, numConverts);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < output->numConverts; ++i)
    {
        convert_t *convert = output->converts[i];
        tileset_group_t *tilesetGroup = convert->tilesetGroup;
        int j;

        if (ret != 0)
        {
            break;
        }

        if (output->format == OUTPUT_FORMAT_APPVAR)
        {
            LL_INFO("Generating \'%s\' for AppVar \'%s\'",
                    convert->name,
                    output->appvar.name);
        }
        else
        {
            LL_INFO("Generating output \'%s\' for \'%s\'",
                    output->name,
                    convert->name);
        }

        for (j = 0; j < convert->numImages; ++j)
        {
            image_t *image = &convert->images[j];
            image->directory =
                strdupcat(output->directory, image->name);

            if (ret != 0)
            {
                break;
            }

            switch (output->format)
            {
                case OUTPUT_FORMAT_C:
                    ret = output_c_image(image);
                    break;

                case OUTPUT_FORMAT_ASM:
                    ret = output_asm_image(image);
                    break;

                case OUTPUT_FORMAT_ICE:
                    ret = output_ice_image(image, output->includeFileName);
                    break;

                case OUTPUT_FORMAT_APPVAR:
                    ret = output_appvar_image(image, &output->appvar);
                    break;

                case OUTPUT_FORMAT_BIN:
                    ret = output_bin_image(image);
                    break;

                default:
                    ret = 1;
                    break;
            }

            free(image->directory);
        }

        if (tilesetGroup != NULL)
        {
            for (j = 0; j < tilesetGroup->numTilesets; ++j)
            {
                tileset_t *tileset = &tilesetGroup->tilesets[j];
                tileset->directory =
                    strdupcat(output->directory, tileset->image.name);

                if (ret != 0)
                {
                    break;
                }

                switch (output->format)
                {
                    case OUTPUT_FORMAT_C:
                        ret = output_c_tileset(tileset);
                        break;

                    case OUTPUT_FORMAT_ASM:
                        ret = output_asm_tileset(tileset);
                        break;

                    case OUTPUT_FORMAT_BIN:
                        ret = output_bin_tileset(tileset);
                        break;

                    case OUTPUT_FORMAT_ICE:
                        ret = output_ice_tileset(tileset, output->includeFileName);
                        break;

                    case OUTPUT_FORMAT_APPVAR:
                        ret = output_appvar_tileset(tileset, &output->appvar);
                        break;

                    default:
                        ret = 1;
                        break;
                }

                free(tileset->directory);
            }
        }
    }

    return ret;
}

/*
 * Output converted palettes into the desired format.
 */
int output_palettes(output_t *output, palette_t **palettes, int numPalettes)
{
    int ret = 0;
    int i;

    if (numPalettes == 0)
    {
        return 0;
    }

    ret = output_find_palettes(output, palettes, numPalettes);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < output->numPalettes; ++i)
    {
        palette_t *palette = output->palettes[i];
        palette->directory =
            strdupcat(output->directory, palette->name);

        if (output->format == OUTPUT_FORMAT_APPVAR)
        {
            LL_INFO("Generating \'%s\' for AppVar \'%s\'",
                    palette->name,
                    output->appvar.name);
        }
        else
        {
            LL_INFO("Generating output \'%s\' for \'%s\'",
                    output->name,
                    palette->name);
        }

        if (ret != 0)
        {
            break;
        }

        switch (output->format)
        {
            case OUTPUT_FORMAT_C:
                ret = output_c_palette(palette);
                break;

            case OUTPUT_FORMAT_ASM:
                ret = output_asm_palette(palette);
                break;

            case OUTPUT_FORMAT_BIN:
                ret = output_bin_palette(palette);
                break;

            case OUTPUT_FORMAT_ICE:
                ret = output_ice_palette(palette, output->includeFileName);
                break;

            case OUTPUT_FORMAT_APPVAR:
                ret = output_appvar_palette(palette, &output->appvar);
                break;

            default:
                ret = 1;
                break;
        }

        free(palette->directory);
    }
    return ret;
}

/*
 * Output a header for using with everything.
 */
int output_include_header(output_t *output)
{
    int ret = 0;

    if (output->numPalettes == 0 && output->numConverts == 0)
    {
        return 0;
    }

    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            ret = output_c_include_file(output);
            break;

        case OUTPUT_FORMAT_ASM:
            ret = output_asm_include_file(output);
            break;

        case OUTPUT_FORMAT_BIN:
            ret = output_bin_include_file(output);
            break;

        case OUTPUT_FORMAT_ICE:
            ret = output_ice_include_file(output, output->includeFileName);
            break;

        case OUTPUT_FORMAT_APPVAR:
            ret = output_appvar_include_file(output, &output->appvar);
            break;

        default:
            ret = 1;
            break;
    }

    if (output->appvar.name != NULL)
    {
        free(output->appvar.directory);
    }

    return ret;
}
