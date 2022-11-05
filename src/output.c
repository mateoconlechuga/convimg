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
        LOG_ERROR("Out of memory.\n");
        return NULL;
    }

    output->directory = strings_dup("");
    if (output->directory == NULL)
    {
        free(output);
        return NULL;
    }

    output->include_file = NULL;
    output->convert_names = NULL;
    output->nr_converts = 0;
    output->converts = NULL;
    output->palette_names = NULL;
    output->palettes = NULL;
    output->nr_palettes = 0;
    output->palette_sizes = false;
    output->order = OUTPUT_PALETTES_FIRST;
    output->format = OUTPUT_FORMAT_INVALID;
    output->constant = "";
    output->appvar.name = NULL;
    output->appvar.archived = true;
    output->appvar.init = true;
    output->appvar.source = APPVAR_SOURCE_NONE;
    output->appvar.compress = COMPRESS_NONE;
    output->appvar.size = 0;
    output->appvar.lut = false;
    output->appvar.header = NULL;
    output->appvar.header_size = 0;
    output->appvar.entry_size = 3;
    output->appvar.data = malloc(APPVAR_MAX_BEFORE_COMPRESSION_SIZE);

    if (output->appvar.data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        free(output);
        return NULL;
    }

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
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    output->convert_names[output->nr_converts] = strings_dup(name);
    if (output->convert_names[output->nr_converts] == NULL)
    {
        return -1;
    }

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
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    output->palette_names[output->nr_palettes] = strings_dup(name);
    if (output->palette_names[output->nr_palettes] == NULL)
    {
        return -1;
    }

    output->nr_palettes++;

    LOG_DEBUG("Added output palette: %s\n", name);

    return 0;
}

void output_free(struct output *output)
{
    uint32_t i;

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

    free(output->directory);
    output->directory = NULL;

    output->nr_converts = 0;
    output->nr_palettes = 0;
}

int output_init(struct output *output)
{
    if (output->nr_converts == 0 && output->nr_palettes == 0)
    {
        LOG_WARNING("No palettes or converts will be generated!\n");
    }

    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            return output_c_init(output);

        case OUTPUT_FORMAT_ASM:
            return output_asm_init(output);

        case OUTPUT_FORMAT_BIN:
            return output_bin_init(output);

        case OUTPUT_FORMAT_ICE:
            return output_ice_init(output);

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_init(output);

        default:
            break;
    }
    
    return -1;
}

int output_find_converts(struct output *output, struct convert **converts, uint32_t nr_converts)
{
    uint32_t i;

    if (converts == NULL || nr_converts == 0)
    {
        return 0;
    }

    output->converts = malloc(output->nr_converts * sizeof(struct convert *));
    if (output->converts == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        uint32_t j;

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

int output_find_palettes(struct output *output, struct palette **palettes, uint32_t nr_palettes)
{
    uint32_t i = 0;

    if (palettes == NULL || nr_palettes == 0)
    {
        goto nopalette;
    }

    output->palettes = malloc(output->nr_palettes * sizeof(struct palette *));
    if (output->palettes == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return -1;
    }

    for (i = 0; i < output->nr_palettes; ++i)
    {
        uint32_t j;

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

int output_converts(struct output *output, struct convert **converts, uint32_t nr_converts)
{
    uint32_t i;
    int ret;

    if (converts == NULL || nr_converts == 0)
    {
        return 0;
    }

    for (i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];
        struct tileset_group *group = convert->tileset_group;
        uint32_t j;

        LOG_INFO("Generating output for convert \'%s\'\n",
            convert->name);

        for (j = 0; j < convert->nr_images; ++j)
        {
            struct image *image = &convert->images[j];

            switch (output->format)
            {
                case OUTPUT_FORMAT_C:
                    ret = output_c_image(output, image);
                    break;

                case OUTPUT_FORMAT_ASM:
                    ret = output_asm_image(output, image);
                    break;

                case OUTPUT_FORMAT_ICE:
                    ret = output_ice_image(output, image);
                    break;

                case OUTPUT_FORMAT_APPVAR:
                    ret = output_appvar_image(output, image);
                    break;

                case OUTPUT_FORMAT_BIN:
                    ret = output_bin_image(output, image);
                    break;

                default:
                    ret = -1;
                    break;
            }

            if (ret)
            {
                return -1;
            }
        }

        if (group != NULL)
        {
            for (j = 0; j < group->nr_tilesets; ++j)
            {
                struct tileset *tileset = &group->tilesets[j];

                switch (output->format)
                {
                    case OUTPUT_FORMAT_C:
                        ret = output_c_tileset(output, tileset);
                        break;

                    case OUTPUT_FORMAT_ASM:
                        ret = output_asm_tileset(output, tileset);
                        break;

                    case OUTPUT_FORMAT_BIN:
                        ret = output_bin_tileset(output, tileset);
                        break;

                    case OUTPUT_FORMAT_ICE:
                        ret = output_ice_tileset(output, tileset);
                        break;

                    case OUTPUT_FORMAT_APPVAR:
                        ret = output_appvar_tileset(output, tileset);
                        break;

                    default:
                        ret = -1;
                        break;
                }

                if (ret)
                {
                    return -1;
                }
            }
        }
    }

    return 0;
}

int output_palettes(struct output *output, struct palette **palettes, uint32_t nr_palettes)
{
    uint32_t i;
    int ret;

    if (palettes == NULL || nr_palettes == 0)
    {
        return 0;
    }

    for (i = 0; i < output->nr_palettes; ++i)
    {
        struct palette *palette = output->palettes[i];

        LOG_INFO("Generating output for palette \'%s\'\n",
            palette->name);

        switch (output->format)
        {
            case OUTPUT_FORMAT_C:
                ret = output_c_palette(output, palette);
                break;

            case OUTPUT_FORMAT_ASM:
                ret = output_asm_palette(output, palette);
                break;

            case OUTPUT_FORMAT_BIN:
                ret = output_bin_palette(output, palette);
                break;

            case OUTPUT_FORMAT_ICE:
                ret = output_ice_palette(output, palette);
                break;

            case OUTPUT_FORMAT_APPVAR:
                ret = output_appvar_palette(output, palette);
                break;

            default:
                ret = -1;
                break;
        }

        if (ret)
        {
            return -1;
        }
    }

    return 0;
}

int output_include(struct output *output)
{
    if (output->nr_palettes == 0 && output->nr_converts == 0)
    {
        return 0;
    }

    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            return output_c_include(output);

        case OUTPUT_FORMAT_ASM:
            return output_asm_include(output);

        case OUTPUT_FORMAT_BIN:
            return output_bin_include(output);

        case OUTPUT_FORMAT_ICE:
            return output_ice_include(output);

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_include(output);

        default:
            break;
    }

    return -1;
}
