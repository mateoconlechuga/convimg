/*
 * Copyright 2017-2026 Matt "MateoConLechuga" Waltz
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
#include "strings.h"
#include "memory.h"
#include "clean.h"
#include "log.h"
#include "thread.h"

#include <errno.h>

#include <stdlib.h>
#include <string.h>

/* external output formats */

int output_c_init(struct output *output);
int output_c_image(struct output *output, const struct image *image);
int output_c_tileset(struct output *output, const struct tileset *tileset);
int output_c_palette(struct output *output, const struct palette *palette);
int output_c_include(struct output *output);

int output_asm_init(struct output *output);
int output_asm_image(struct output *output, const struct image *image);
int output_asm_tileset(struct output *output, const struct tileset *tileset);
int output_asm_palette(struct output *output, const struct palette *palette);
int output_asm_include(struct output *output);

int output_bin_init(struct output *output);
int output_bin_image(struct output *output, const struct image *image);
int output_bin_tileset(struct output *output, const struct tileset *tileset);
int output_bin_palette(struct output *output, const struct palette *palette);
int output_bin_include(struct output *output);

int output_basic_init(struct output *output);
int output_basic_image(struct output *output, const struct image *image);
int output_basic_tileset(struct output *output, const struct tileset *tileset);
int output_basic_palette(struct output *output, const struct palette *palette);
int output_basic_include(struct output *output);

int output_appvar_init(struct output *output);
int output_appvar_image(struct output *output, const struct image *image);
int output_appvar_tileset(struct output *output, const struct tileset *tileset);
int output_appvar_palette(struct output *output, const struct palette *palette);
int output_appvar_include(struct output *output);

struct output *output_alloc(void)
{
    struct output *output = memory_alloc(sizeof(struct output));
    if (output == NULL)
    {
        return NULL;
    }

    output->directory = NULL;
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
    output->appvar.archived = true;
    output->appvar.init = true;
    output->appvar.source = APPVAR_SOURCE_NONE;
    output->appvar.compress = COMPRESS_NONE;
    output->appvar.size = 0;
    output->appvar.lut = false;
    output->appvar.header = NULL;
    output->appvar.header_size = 0;
    output->appvar.entry_size = 3;
    output->appvar.data = NULL;

    memset(output->appvar.comment, 0, APPVAR_MAX_COMMENT_SIZE + 1);
    memset(output->appvar.name, 0, APPVAR_MAX_NAME_SIZE + 1);

    output->directory = strings_dup("");
    if (output->directory == NULL)
    {
        goto error;
    }

    output->appvar.data = memory_alloc(APPVAR_MAX_BEFORE_COMPRESSION_SIZE);
    if (output->appvar.data == NULL)
    {
        goto error;
    }

    return output;

error:
    free(output);
    output = NULL;
    return NULL;
}

int output_add_convert_name(struct output *output, const char *name)
{
    output->convert_names = memory_realloc_array(output->convert_names, output->nr_converts + 1, sizeof(char *));
    if (output->convert_names == NULL)
    {
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

int output_add_palette_name(struct output *output, const char *name)
{
    output->palette_names = memory_realloc_array(output->palette_names, output->nr_palettes + 1, sizeof(char *));
    if (output->palette_names == NULL)
    {
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
    if (output == NULL)
    {
        return;
    }

    for (uint32_t i = 0; i < output->nr_converts; ++i)
    {
        free(output->convert_names[i]);
        output->convert_names[i] = NULL;
    }
    output->nr_converts = 0;

    for (uint32_t i = 0; i < output->nr_palettes; ++i)
    {
        free(output->palette_names[i]);
        output->palette_names[i] = NULL;
    }
    output->nr_palettes = 0;

    free(output->convert_names);
    output->convert_names = NULL;

    free(output->palette_names);
    output->palette_names = NULL;

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
}

static int output_init(struct output *output)
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

        case OUTPUT_FORMAT_BASIC:
            return output_basic_init(output);

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_init(output);

        default:
            break;
    }
    
    return -1;
}

static int output_find_converts(struct output *output, struct convert **converts, uint32_t nr_converts)
{
    if (converts == NULL || nr_converts == 0)
    {
        return 0;
    }

    output->converts = memory_realloc_array(NULL, output->nr_converts, sizeof(struct convert *));
    if (output->converts == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < output->nr_converts; ++i)
    {
        for (uint32_t j = 0; j < nr_converts; ++j)
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

static int output_find_palettes(struct output *output, struct palette **palettes, uint32_t nr_palettes)
{
    if (palettes == NULL || nr_palettes == 0)
    {
        LOG_ERROR("No palettes defined for output.\n");
        return -1;
    }

    output->palettes = memory_realloc_array(NULL, output->nr_palettes, sizeof(struct palette *));
    if (output->palettes == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < output->nr_palettes; ++i)
    {
        for (uint32_t j = 0; j < nr_palettes; ++j)
        {
            if (strcmp(output->palette_names[i], palettes[j]->name) == 0)
            {
                output->palettes[i] = palettes[j];
                goto nextpalette;
            }
        }

        LOG_ERROR("No matching palette name \'%s\' found for output.\n",
            output->palette_names[i]);
        return -1;

nextpalette:
        continue;
    }

    return 0;
}

static int output_image(struct output *output, const struct image *image)
{
    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            return output_c_image(output, image);

        case OUTPUT_FORMAT_ASM:
            return output_asm_image(output, image);

        case OUTPUT_FORMAT_BASIC:
            return output_basic_image(output, image);

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_image(output, image);

        case OUTPUT_FORMAT_BIN:
            return output_bin_image(output, image);

        default:
            return -1;
    }
}

static int output_tileset(struct output *output, const struct tileset *tileset)
{
    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            return output_c_tileset(output, tileset);

        case OUTPUT_FORMAT_ASM:
            return output_asm_tileset(output, tileset);

        case OUTPUT_FORMAT_BIN:
            return output_bin_tileset(output, tileset);

        case OUTPUT_FORMAT_BASIC:
            return output_basic_tileset(output, tileset);

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_tileset(output, tileset);

        default:
            return -1;
    }
}

static int output_palette(struct output *output, const struct palette *palette)
{
    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            return output_c_palette(output, palette);

        case OUTPUT_FORMAT_ASM:
            return output_asm_palette(output, palette);

        case OUTPUT_FORMAT_BIN:
            return output_bin_palette(output, palette);

        case OUTPUT_FORMAT_BASIC:
            return output_basic_palette(output, palette);

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_palette(output, palette);

        default:
            return -1;
    }
}

static bool output_include(void *arg)
{
    struct output *output = arg;

    if (output->nr_palettes == 0 && output->nr_converts == 0)
    {
        return true;
    }

    switch (output->format)
    {
        case OUTPUT_FORMAT_C:
            return output_c_include(output) ? false : true;

        case OUTPUT_FORMAT_ASM:
            return output_asm_include(output) ? false : true;

        case OUTPUT_FORMAT_BIN:
            return output_bin_include(output) ? false : true;

        case OUTPUT_FORMAT_BASIC:
            return output_basic_include(output) ? false : true;

        case OUTPUT_FORMAT_APPVAR:
            return output_appvar_include(output) ? false : true;

        default:
            return false;
    }
}

static int output_converts(struct output *output)
{
    for (uint32_t i = 0; i < output->nr_converts; ++i)
    {
        struct convert *convert = output->converts[i];

        LOG_INFO("Generating output for convert \'%s\'\n",
            convert->name);

        for (uint32_t j = 0; j < convert->nr_images; ++j)
        {
            const struct image *image = &convert->images[j];

            if (output_image(output, image))
            {
                return -1;
            }
        }

        for (uint32_t j = 0; j < convert->nr_tilesets; ++j)
        {
            const struct tileset *tileset = &convert->tilesets[j];

            if (output_tileset(output, tileset))
            {
                return -1;
            }
        }
    }

    return 0;
}

static int output_palettes(struct output *output)
{
    for (uint32_t i = 0; i < output->nr_palettes; ++i)
    {
        const struct palette *palette = output->palettes[i];

        LOG_INFO("Generating output for palette \'%s\'\n",
            palette->name);

        if (output_palette(output, palette))
        {
            return -1;
        }
    }

    return 0;
}

int output_generate(struct output *output,
                    struct palette **palettes,
                    uint32_t nr_palettes,
                    struct convert **converts,
                    uint32_t nr_converts)
{
    if (output_find_palettes(output, palettes, nr_palettes))
    {
        return -1;
    }

    if (output_find_converts(output, converts, nr_converts))
    {
        return -1;
    }

    if (output_init(output))
    {
        return -1;
    }

    if (output->order == OUTPUT_PALETTES_FIRST)
    {
        if (output_palettes(output))
        {
            return -1;
        }

        if (output_converts(output))
        {
            return -1;
        }
    }
    else
    {
        if (output_converts(output))
        {
            return -1;
        }

        if (output_palettes(output))
        {
            return -1;
        }
    }

    if (!thread_start(output_include, output))
    {
        return -1;
    }

    return 0;
}
