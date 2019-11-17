/*
 * Copyright 2017-2019 Matt "MateoConLechuga" Waltz
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

#include "yaml.h"
#include "strings.h"
#include "log.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#define REALLOC_BYTES 128
#define MAX_LINE_SIZE 1048576

/*
 * Read a line from an input file; strips whitespace.
 * Returns NULL if error.
 */
static char *ymal_get_line(FILE *fdi)
{
    char *line = NULL;
    int i = 0;
    int c;

    do
    {
        c = fgetc(fdi);

        if (c < ' ')
        {
            c = 0;
        }

        if (i % REALLOC_BYTES == 0)
        {
            line = realloc(line, ((i / REALLOC_BYTES) + 1) * REALLOC_BYTES);
            if (line == NULL)
            {
                LL_ERROR("%s", strerror(errno));
                return NULL;
            }
        }

        line[i] = (char)c;
        i++;

        if (i >= MAX_LINE_SIZE)
        {
            LL_ERROR("Input line too large.");
            free(line);
            return NULL;
        }
    } while (c);

    return line;
}

/*
 * Allocates a palette structure.
 */
int yaml_alloc_palette(yaml_file_t *yamlfile)
{
    palette_t *tmpPalette;

    yamlfile->palettes =
        realloc(yamlfile->palettes, (yamlfile->numPalettes + 1) * sizeof(palette_t *));
    if (yamlfile->palettes == NULL)
    {
        return 1;
    }

    LL_DEBUG("Allocating palette...");

    tmpPalette = palette_alloc();
    if (tmpPalette == NULL)
    {
        return 1;
    }

    yamlfile->curPalette = tmpPalette;
    yamlfile->palettes[yamlfile->numPalettes] = tmpPalette;
    yamlfile->numPalettes++;

    return 0;
}

/*
 * Allocates convert structure.
 */
int yaml_alloc_convert(yaml_file_t *yamlfile)
{
    convert_t *tmpConvert;

    yamlfile->converts =
        realloc(yamlfile->converts, (yamlfile->numConverts + 1) * sizeof(convert_t *));
    if (yamlfile->converts == NULL)
    {
        return 1;
    }

    LL_DEBUG("Allocating convert...");

    tmpConvert = convert_alloc();
    if (tmpConvert == NULL)
    {
        return 1;
    }

    yamlfile->curConvert = tmpConvert;
    yamlfile->converts[yamlfile->numConverts] = tmpConvert;
    yamlfile->numConverts++;

    return 0;
}

/*
 * Allocates output structure.
 */
int yaml_alloc_output(yaml_file_t *yamlfile)
{
    output_t *tmpOutput;

    yamlfile->outputs =
        realloc(yamlfile->outputs, (yamlfile->numOutputs + 1) * sizeof(output_t *));
    if (yamlfile->outputs == NULL)
    {
        return 1;
    }

    LL_DEBUG("Allocating output...");

    tmpOutput = output_alloc();
    if (tmpOutput == NULL)
    {
        return 1;
    }

    yamlfile->curOutput = tmpOutput;
    yamlfile->outputs[yamlfile->numOutputs] = tmpOutput;
    yamlfile->numOutputs++;

    return 0;
}

/*
 * Finds values from key.
 */
static char *yaml_get_args(const char *delim)
{
    char *args = strtok(NULL, delim);

    if (args != NULL)
    {
        args = strings_trim(args);
    }

    return args;
}

/*
 * Takes a line that has braces and converts to a color.
 */
static int yaml_parse_fixed_color(yaml_file_t *yamlfile, char *line, palette_entry_t *entry)
{
    char *args = strchr(line, '{');

    if (args == NULL)
    {
        goto error;
    }
    else
    {
        char *next = ++args;
        char *end;

        end = strrchr(next, '}');
        if (end == NULL)
        {
            goto error;
        }

        *end = '\0';
        entry->color.rgb.a = 255;

        while (next != NULL)
        {
            char *current = next;
            char *key;
            char *value;

            next = strchr(next, ',');
            if (next != NULL)
            {
                *next = '\0';
                next++;
            }

            key = strtok(current, ":");
            if (key == NULL)
            {
                goto error;
            }

            value = strtok(NULL, ":");
            if (value == NULL)
            {
                goto error;
            }

            value = strings_trim(value);
            key = strings_trim(key);
            if (key == NULL || value == NULL)
            {
                goto error;
            }

            if (!strcmp(key, "index"))
            {
                entry->index = strtol(value, NULL, 0);
            }
            else if (!strcmp(key, "r"))
            {
                entry->color.rgb.r = strtol(value, NULL, 0);
            }
            else if (!strcmp(key, "g"))
            {
                entry->color.rgb.g = strtol(value, NULL, 0);
            }
            else if (!strcmp(key, "b"))
            {
                entry->color.rgb.b = strtol(value, NULL, 0);
            }
        }
    }

    return 0;

error:
    LL_ERROR("Invalid color format specifier (line %d).",
        yamlfile->line);
    return 1;
}

/*
 * Takes a line that has braces and converts to a tileset.
 */
static int yaml_parse_tileset(yaml_file_t *yamlfile, char *line, tileset_t *tileset)
{
    char *args = strchr(line, '{');

    if (args == NULL)
    {
        goto error;
    }
    else
    {
        char *next = ++args;
        char *end;

        end = strrchr(next, '}');
        if (end == NULL)
        {
            goto error;
        }

        *end = '\0';

        while (next != NULL)
        {
            char *current = next;
            char *key;
            char *value;

            next = strchr(next, ',');
            if (next != NULL)
            {
                *next = '\0';
                next++;
            }

            key = strtok(current, ":");
            if (key == NULL)
            {
                goto error;
            }

            value = strtok(NULL, ":");
            if (value == NULL)
            {
                goto error;
            }

            value = strings_trim(value);
            key = strings_trim(key);
            if (key == NULL || value == NULL)
            {
                goto error;
            }

            if (!strcmp(key, "tile-width"))
            {
                tileset->tileWidth = strtol(value, NULL, 0);
            }
            else if (!strcmp(key, "tile-height"))
            {
                tileset->tileHeight = strtol(value, NULL, 0);
            }
            else if (!strcmp(key, "ptable"))
            {
                tileset->pTable = !strcmp(value, "true");
            }
        }
    }

    tileset->enabled = true;

    return 0;

error:
    LL_ERROR("Invalid tileset format specifier (line %d).",
        yamlfile->line);
    return 1;
}

/*
 * Checks if there is a new command and switches state.
 */
static int yaml_get_command(yaml_file_t *yamlfile, char *command, char *line)
{
    int ret = 0;

    if (command == NULL || line == NULL)
    {
        return 0;
    }

    command = strings_trim(command);
    if (command == NULL)
    {
        return 1;
    }

    if (!strcmp(command, "palette"))
    {
        yamlfile->state = YAML_ST_PALETTE;
        ret = yaml_alloc_palette(yamlfile);
    }
    if (!strcmp(command, "convert"))
    {
        yamlfile->state = YAML_ST_CONVERT;
        ret = yaml_alloc_convert(yamlfile);
    }
    if (!strcmp(command, "output"))
    {
        yamlfile->state = YAML_ST_OUTPUT;
        ret = yaml_alloc_output(yamlfile);
    }

    return ret;
}

/*
 * Parses available pallete commands.
 */
static int yaml_palette_command(yaml_file_t *yamlfile, char *command, char *line)
{
    palette_t *palette = yamlfile->curPalette;
    palette_entry_t entry;
    int ret = 0;
    char *args;

    if (command == NULL || line == NULL)
    {
        return 0;
    }

    command = strings_trim(command);
    args = yaml_get_args(":");

    LL_DEBUG("Palette Command: %s:%s", command, args);

    if (command[0] == '-')
    {
        if (palette->automatic)
        {
            LL_ERROR("Cannot specify image sources in automatic mode (line %d).",
                 yamlfile->line);
            ret = 1;
        }
        else
        {
            ret = pallete_add_path(palette, strings_trim(&command[1]));
        }
    }
    else if (!strcmp(command, "palette"))
    {
        if (args != NULL)
        {
            palette->name = strdup(args);
        }
        else
        {
            LL_ERROR("Missing name for generate-palette command (line %d).",
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "max-entries"))
    {
        palette->maxEntries = strtol(args, NULL, 0);
    }
    else if (!strcmp(command, "speed"))
    {
        if (args != NULL)
        {
            palette->quantizeSpeed = strtol(args, NULL, 0);
        }
        if (args == NULL || palette->quantizeSpeed < 1 || palette->quantizeSpeed > 10)
        {
            LL_WARNING("Ignoring invalid quantization speed (line %d).",
                yamlfile->line);
            palette->quantizeSpeed = PALETTE_DEFAULT_QUANTIZE_SPEED;
        }
    }
    else if (!strcmp(command, "fixed-color"))
    {
        ret = yaml_parse_fixed_color(yamlfile, line, &entry);
        palette->entries[palette->numFixedEntries] = entry;
        palette->numFixedEntries++;
    }
    else if (!strcmp(command, "transparent-color"))
    {
        ret = yaml_parse_fixed_color(yamlfile, line, &entry);
        palette->entries[palette->numFixedEntries] = entry;
        palette->transparentFixedEntry = palette->numFixedEntries;
        palette->numFixedEntries++;
    }
    else if (!strcmp(command, "images"))
    {
        if (args != NULL && !strcmp(args, "automatic"))
        {
            palette->automatic = true;
        }
    }
    else
    {
        LL_WARNING("Ignoring invalid line %d.",
            yamlfile->line);
    }

    return ret;
}

/*
 * Parses available conversion commands.
 */
static int yaml_convert_command(yaml_file_t *yamlfile, char *command, char *line)
{
    convert_t *convert = yamlfile->curConvert;
    char *args;
    int ret = 0;

    if (command == NULL || line == NULL)
    {
        return 0;
    }

    command = strings_trim(command);
    args = yaml_get_args(":");

    LL_DEBUG("Convert Command: %s:%s", command, args);

    if (command[0] == '-')
    {
        ret = convert_add_path(convert, strings_trim(&command[1]));
    }
    else if (!strcmp(command, "convert"))
    {
        if (args != NULL)
        {
            convert->name = strdup(args);
        }
        else
        {
            LL_ERROR("Missing name for convert command (line %d).",
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "palette"))
    {
        if (args != NULL)
        {
            if (convert->paletteName != NULL)
            {
                free(convert->paletteName);
                convert->paletteName = NULL;
            }
            convert->paletteName = strdup(args);
        }
        else
        {
            LL_ERROR("Missing palette name (line %d).",
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "compress"))
    {
        if (args != NULL && !strcmp(args, "zx7"))
        {
            convert->compression = COMPRESS_ZX7;
        }
        else
        {
            LL_ERROR("Invalid compression argument for palette \'%s\' (line %d).",
                convert->name,
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "tileset"))
    {
        ret = yaml_parse_tileset(yamlfile, line, &convert->tileset);
    }
    else if (!strcmp(command, "width-and-height"))
    {
        convert->widthAndHeight = args != NULL && !strcmp(args, "true");
    }
    else if (!strcmp(command, "omit-palette-index"))
    {
        convert->omitIndices[convert->numOmitIndices] = strtol(args, NULL, 0);
        convert->numOmitIndices++;
    }
    else if (!strcmp(command, "width-and-height"))
    {
        convert->widthAndHeight = args != NULL && !strcmp(args, "true");
    }
    else if (!strcmp(command, "bpp"))
    {
        if (args == NULL)
        {
            LL_ERROR("Invalid bpp argument for palette \'%s\' (line %d).",
                convert->name,
                yamlfile->line);
            ret = 1;
        }
        else if (!strcmp(args, "8"))
        {
            convert->bpp = BPP_8;
        }
        else if (!strcmp(args, "4"))
        {
            convert->bpp = BPP_4;
        }
        else if (!strcmp(args, "2"))
        {
            convert->bpp = BPP_2;
        }
        else if (!strcmp(args, "1"))
        {
            convert->bpp = BPP_1;
        }
        else
        {
            LL_ERROR("Invalid bpp argument for palette \'%s\' (line %d).",
                convert->name,
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "images"))
    {
        if (args != NULL && !strcmp(args, "automatic"))
        {
            ret = convert_add_path(convert, "*");
        }
    }
    else
    {
        LL_WARNING("Ignoring invalid line %d.",
            yamlfile->line);
    }

    return ret;
}

/*
 * Gets compression mode from string.
 */
compression_t yaml_get_compress_mode(yaml_file_t *yamlfile, char *arg)
{
    compression_t compress = COMPRESS_INVALID;

    if (!strcmp(arg, "zx7"))
    {
        compress = COMPRESS_ZX7;
    }
    else
    {
        LL_WARNING("Unknown compression mode (line %d).",
            yamlfile->line);
    }

    return compress;
}

/*
 * Parses available conversion commands.
 */
static int yaml_output_command(yaml_file_t *yamlfile, char *command, char *line)
{
    static yaml_output_mode_t outputMode = YAML_OUTPUT_CONVERTS;
    output_t *output = yamlfile->curOutput;
    char *args;
    int ret = 0;

    if (command == NULL || line == NULL)
    {
        return 0;
    }

    command = strings_trim(command);
    args = yaml_get_args(":");

    LL_DEBUG("Output Command: %s:%s", command, args);

    if (command[0] == '-')
    {
        if (outputMode == YAML_OUTPUT_PALETTES)
        {
            ret = output_add_palette(output, strings_trim(&command[1]));
        }
        else
        {
            ret = output_add_convert(output, strings_trim(&command[1]));
        }
    }
    else if (!strcmp(command, "output"))
    {
        if (args != NULL)
        {
            output->name = strdup(args);
            if (!strcmp(args, "c"))
            {
                output->format = OUTPUT_FORMAT_C;
            }
            else if (!strcmp(args, "asm"))
            {
                output->format = OUTPUT_FORMAT_ASM;
            }
            else if (!strcmp(args, "ice"))
            {
                output->format = OUTPUT_FORMAT_ICE;
            }
            else if (!strcmp(args, "appvar"))
            {
                output->format = OUTPUT_FORMAT_APPVAR;
            }
            else if (!strcmp(args, "bin"))
            {
                output->format = OUTPUT_FORMAT_BIN;
            }
        }
        else
        {
            LL_ERROR("Missing output format (line %d).",
                yamlfile->line);
            ret = 1;
        }
    }
    else if (output->format == OUTPUT_FORMAT_APPVAR)
    {
        if (!strcmp(command, "name"))
        {
            output->appvar.name = strdup(args);
        }
        else if (!strcmp(command, "archived"))
        {
            output->appvar.archived = !strcmp(args, "true");
        }
        else if (!strcmp(command, "source-init"))
        {
            output->appvar.init = !strcmp(args, "true");
        }
        else if (!strcmp(command, "source-format"))
        {
            if (!strcmp(args, "c"))
            {
                output->appvar.source = APPVAR_SOURCE_C;
            }
            else if (!strcmp(args, "asm"))
            {
                output->appvar.source = APPVAR_SOURCE_ASM;
            }
            else if (!strcmp(args, "ice"))
            {
                output->appvar.source = APPVAR_SOURCE_ICE;
            }
            else
            {
                LL_ERROR("Unknown AppVar source format (line %d).",
                    yamlfile->line);
                ret = 1;
            }
        }
        else if (!strcmp(command, "include-file"))
        {
            if (args != NULL)
            {
                output->includeFileName = strdup(args);
            }
            else
            {
                LL_ERROR("Missing include file name on line %d.",
                    yamlfile->line);
                ret = 1;
            }
        }
        else if (!strcmp(command, "compress"))
        {
            output->compress = yaml_get_compress_mode(yamlfile, args);
        }
        else if (!strcmp(command, "converts"))
        {
            outputMode = YAML_OUTPUT_CONVERTS;
        }
        else if (!strcmp(command, "palettes"))
        {
            outputMode = YAML_OUTPUT_PALETTES;
        }
        else
        {
            LL_WARNING("Ignoring invalid line %d.",
                yamlfile->line);
        }
    }
    else if (!strcmp(command, "include-file"))
    {
        if (args != NULL)
        {
            output->includeFileName = strdup(args);
        }
        else
        {
            LL_ERROR("Missing include file name on line %d.",
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "name"))
    {
        if (args != NULL)
        {
            output->name = strdup(args);
        }
        else
        {
            LL_ERROR("Missing name on line %d.",
                yamlfile->line);
            ret = 1;
        }
    }
    else if (!strcmp(command, "compress"))
    {
        output->compress = yaml_get_compress_mode(yamlfile, args);
    }
    else if (!strcmp(command, "palettes"))
    {
        outputMode = YAML_OUTPUT_PALETTES;
    }
    else if (!strcmp(command, "converts"))
    {
        outputMode = YAML_OUTPUT_CONVERTS;
    }
    else
    {
        LL_WARNING("Ignoring invalid line %d.",
            yamlfile->line);
    }

    return ret;
}

/*
 * Parses a YAML file and stores the results to a structure.
 */
int yaml_parse_file(yaml_file_t *yamlfile)
{
    char *line;
    FILE *fdi;
    int ret = 0;

    if (yamlfile == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        return 1;
    }

    LL_INFO("Reading file \'%s\'", yamlfile->name);

    fdi = fopen(yamlfile->name, "rb");
    if( fdi == NULL )
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        LL_ERROR("Use --help option if needed.");
        return 1;
    }

    yamlfile->line = 1;
    yamlfile->palettes = NULL;
    yamlfile->converts = NULL;
    yamlfile->outputs = NULL;
    yamlfile->numPalettes = 0;
    yamlfile->numConverts = 0;
    yamlfile->numOutputs = 0;
    yamlfile->state = YAML_ST_INIT;

    do
    {
        char *command;
        char *fullLine;

        line = ymal_get_line(fdi);
        if (line == NULL)
        {
            break;
        }

        if (line[0] == '#')
        {
            free(line);
            continue;
        }

        fullLine = strdup(line);
        command = strtok(line, ":");

        if (isspace(line[0]) == 0)
        {
            ret = yaml_get_command(yamlfile, command, fullLine);
            if (ret == 1)
            {
                break;
            }
        }

        switch (yamlfile->state)
        {
            case YAML_ST_PALETTE:
                ret = yaml_palette_command(yamlfile, command, fullLine);
                break;

            case YAML_ST_CONVERT:
                ret = yaml_convert_command(yamlfile, command, fullLine);
                break;

            case YAML_ST_OUTPUT:
                ret = yaml_output_command(yamlfile, command, fullLine);
                break;

            default:
                break;
        }

        yamlfile->line++;

        free(fullLine);
        free(line);
    } while (ret == 0 && !feof(fdi));

    fclose(fdi);

    return ret;
}

/*
 * Frees structures in yaml.
 */
void yaml_release_file(yaml_file_t *yamlfile)
{
    int i;

    for (i = 0; i < yamlfile->numOutputs; ++i)
    {
        output_free(yamlfile->outputs[i]);
        free(yamlfile->outputs[i]);
        yamlfile->outputs[i] = NULL;
    }
    free(yamlfile->outputs);
    yamlfile->outputs = NULL;

    for (i = 0; i < yamlfile->numConverts; ++i)
    {
        convert_free(yamlfile->converts[i]);
        free(yamlfile->converts[i]);
        yamlfile->converts[i] = NULL;
    }
    free(yamlfile->converts);
    yamlfile->converts = NULL;

    for (i = 0; i < yamlfile->numPalettes; ++i)
    {
        palette_free(yamlfile->palettes[i]);
        free(yamlfile->palettes[i]);
        yamlfile->palettes[i] = NULL;
    }
    free(yamlfile->palettes);
    yamlfile->palettes = NULL;
}
