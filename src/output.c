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
#include "output-formats.h"
#include "strings.h"
#include "log.h"
#include "clean.h"

#include <errno.h>

#include <stdlib.h>
#include <string.h>

struct output *output_alloc(void)
{
    struct output *output = malloc(sizeof(struct output));
    if (output == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return NULL;
    }

    output->name = NULL;
    output->include_file = NULL;
    output->directory = strdup("");
    output->convert_names = NULL;
    output->nr_converts = 0;
    output->converts = NULL;
    output->palette_names = NULL;
    output->palettes = NULL;
    output->nr_palettes = 0;
    output->palette_sizes = false;
    output->order = OUTPUT_PALETTES_FIRST;
    output->format = OUTPUT_FORMAT_INVALID;
    output->appvar.name = NULL;
    output->appvar.directory = NULL;
    output->appvar.archived = true;
    output->appvar.init = true;
    output->appvar.source = APPVAR_SOURCE_NONE;
    output->appvar.compress = COMPRESS_NONE;
    output->appvar.data = malloc(APPVAR_MAX_BEFORE_COMPRESSION_SIZE);
    output->appvar.size = 0;
    output->appvar.lut = false;
    output->appvar.header = NULL;
    output->appvar.header_size = 0;
    output->appvar.entry_size = 3;

    return output;
}

int output_add_convert(struct output *output, const char *name)
{
    if (output == NULL ||
        name == NULL ||
        output->format == OUTPUT_FORMAT_INVALID)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    output->convert_names =
        realloc(output->convert_names, (output->nr_converts + 1) * sizeof(char *));
    if (output->convert_names == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    output->convert_names[output->nr_converts] = strdup(name);
    output->nr_converts++;

    LOG_DEBUG("Added output convert: %s\n", name);

    return 0;
}

int output_add_palette(struct output *output, const char *name)
{
    if (output == NULL ||
        name == NULL ||
        output->format == OUTPUT_FORMAT_INVALID)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    output->palette_names =
        realloc(output->palette_names, (output->nr_palettes + 1) * sizeof(char *));
    if (output->palette_names == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    output->palette_names[output->nr_palettes] = strdup(name);
    output->nr_palettes++;

    LOG_DEBUG("Added output palette: %s\n", name);

    return 0;
}

void output_free(struct output *output)
{
    int i;

    if (output == NULL)
    {
        return;
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        free(output->convert_names[i]);
        output->convert_names[i] = NULL;
    }

    for (i = 0; i < output->nr_palettes; ++i)
    {
        free(output->palette_names[i]);
        output->palette_names[i] = NULL;
    }

    if (output->appvar.name != NULL)
    {
        free(output->appvar.directory);
    }

    free(output->convert_names);
    output->convert_names = NULL;

    free(output->palette_names);
    output->palette_names = NULL;

    free(output->appvar.name);
    output->appvar.name = NULL;

    free(output->appvar.header);
    output->appvar.header = NULL;

    free(output->appvar.data);
    output->appvar.data = NULL;

    free(output->converts);
    output->converts = NULL;

    free(output->palettes);
    output->palettes = NULL;

    free(output->include_file);
    output->include_file = NULL;

    free(output->name);
    output->name = NULL;

    free(output->directory);
    output->directory = NULL;

    output->nr_converts = 0;
    output->nr_palettes = 0;
}

int output_init(struct output *output)
{
    if (output->nr_converts == 0 && output->nr_palettes == 0)
    {
        LOG_WARNING("No palettes or converts will be output!\n");
    }

    if (output->format == OUTPUT_FORMAT_ICE)
    {
        char *tmp = strdupcat(output->directory, output->include_file);
        if (tmp != NULL)
        {
            if (remove(tmp))
            {
                LOG_ERROR("Could not remove output file: %s\n",
                    strerror(errno));
            }
            
            free(tmp);
        }
    }

    if (output->format == OUTPUT_FORMAT_APPVAR)
    {
        output_appvar_header(output, &output->appvar);
    }

    if (output->appvar.name != NULL)
    {
        output->appvar.directory =
            strdupcat(output->directory, output->appvar.name);
    }

    return 0;
}

int output_find_converts(struct output *output, struct convert **converts, int nr_converts)
{
    int i, j;

    if (output == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    if (converts == NULL || nr_converts == 0)
    {
        return 0;
    }

    output->converts = malloc(output->nr_converts * sizeof(struct convert *));
    if (output->converts == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        for (j = 0; j < nr_converts; ++j)
        {
            if (strcmp(output->convert_names[i], converts[j]->name) == 0)
            {
                output->converts[i] = converts[j];
                goto nextconvert;
            }
        }

        LOG_ERROR("No matching convert name \'%s\' found for output.\n",
                 output->convert_names[i]);
        return -1;

nextconvert:
        continue;
    }

    return 0;
}

int output_find_palettes(struct output *output, struct palette **palettes, int nr_palettes)
{
    int i = 0;

    if (output == NULL || nr_palettes < 0)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    if (palettes == NULL || nr_palettes == 0)
    {

        goto nopalette;
    }

    output->palettes = malloc(output->nr_palettes * sizeof(struct palette *));
    if (output->palettes == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    for (i = 0; i < output->nr_palettes; ++i)
    {
        int j;
        for (j = 0; j < nr_palettes; ++j)
        {
            if (strcmp(output->palette_names[i], palettes[j]->name) == 0)
            {
                output->palettes[i] = palettes[j];
                goto nextpalette;
            }
        }

nopalette:
        LOG_ERROR("No matching palette name \'%s\' found for output.\n",
            output->palette_names[i]);
        return -1;

nextpalette:
        continue;
    }

    return 0;
}

int output_converts(struct output *output, struct convert **converts, int nr_converts)
{
    int ret;
    int i;

    if (output == NULL || nr_converts < 0)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    if (converts == NULL || nr_converts == 0)
    {
        return 0;
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *group = convert->tileset_group;
        int j;

        LOG_INFO("Generating output for convert \'%s\'\n",
            convert->name);

        for (j = 0; j < convert->nr_images; ++j)
        {
            struct image *image = &convert->images[j];
            image->directory =
                strdupcat(output->directory, image->name);

            switch (output->format)
            {
                case OUTPUT_FORMAT_C:
                    ret = output_c_image(image);
                    break;

                case OUTPUT_FORMAT_ASM:
                    ret = output_asm_image(image);
                    break;

                case OUTPUT_FORMAT_ICE:
                    ret = output_ice_image(image, output->include_file);
                    break;

                case OUTPUT_FORMAT_APPVAR:
                    ret = output_appvar_image(image, &output->appvar);
                    break;

                case OUTPUT_FORMAT_BIN:
                    ret = output_bin_image(image);
                    break;

                default:
                    ret = -1;
                    break;
            }

            free(image->directory);

            if (ret != 0)
            {
                return ret;
            }
        }

        if (group != NULL)
        {
            for (j = 0; j < group->nr_tilesets; ++j)
            {
                struct tileset *tileset = &group->tilesets[j];
                tileset->directory =
                    strdupcat(output->directory, tileset->image.name);

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
                        ret = output_ice_tileset(tileset, output->include_file);
                        break;

                    case OUTPUT_FORMAT_APPVAR:
                        ret = output_appvar_tileset(tileset, &output->appvar);
                        break;

                    default:
                        ret = -1;
                        break;
                }

                free(tileset->directory);

                if (ret != 0)
                {
                    return ret;
                }
            }
        }
    }

    return 0;
}

int output_palettes(struct output *output, struct palette **palettes, int nr_palettes)
{
    int ret;
    int i;

    if (output == NULL || nr_palettes < 0)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    if (palettes == NULL || nr_palettes == 0)
    {
        return 0;
    }

    for (i = 0; i < output->nr_palettes; ++i)
    {
        struct palette *palette = output->palettes[i];
        palette->directory =
            strdupcat(output->directory, palette->name);
        palette->include_size = output->palette_sizes;

        LOG_INFO("Generating output for palette \'%s\'\n",
            palette->name);

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
                ret = output_ice_palette(palette, output->include_file);
                break;

            case OUTPUT_FORMAT_APPVAR:
                ret = output_appvar_palette(palette, &output->appvar);
                break;

            default:
                ret = -1;
                break;
        }

        free(palette->directory);

        if (ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

int output_include_header(struct output *output)
{
    int ret = 0;

    if (output->nr_palettes == 0 && output->nr_converts == 0)
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
            ret = output_ice_include_file(output, output->include_file);
            break;

        case OUTPUT_FORMAT_APPVAR:
            ret = output_appvar_include_file(output, &output->appvar);
            break;

        default:
            ret = -1;
            break;
    }

    return ret;
}
