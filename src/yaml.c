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
#include "log.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#define REALLOC_BYTES 128

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

        if (!isspace(c))
        {
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
        realloc(yamlfile->palettes, (yamlfile->numPalettes + 1) * sizeof(palette_t));
    if (yamlfile->palettes == NULL)
    {
        return 1;
    }

    tmpPalette = palette_alloc();
    if (tmpPalette == NULL)
    {
        return 1;
    }

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
        realloc(yamlfile->converts, (yamlfile->numConverts + 1) * sizeof(convert_t));
    if (yamlfile->palettes == NULL)
    {
        return 1;
    }

    tmpConvert = convert_alloc();
    if (tmpConvert == NULL)
    {
        return 1;
    }

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
        realloc(yamlfile->outputs, (yamlfile->numOutputs + 1) * sizeof(output_t));
    if (yamlfile->outputs == NULL)
    {
        return 1;
    }

    tmpOutput = output_alloc();
    if (tmpOutput == NULL)
    {
        return 1;
    }

    yamlfile->outputs[yamlfile->numOutputs] = tmpOutput;
    yamlfile->numOutputs++;

    return 0;
}

/*
 * Checks if there is a new command and switches state.
 */
int yaml_get_command(yaml_file_t *yamlfile, const char *arg)
{
    int ret = 0;

    if (!strcmp(arg, "generate-palette"))
    {
        yamlfile->state = YAML_ST_PALETTE;
        ret = yaml_alloc_palette(yamlfile);
    }
    if (!strcmp(arg, "convert"))
    {
        yamlfile->state = YAML_ST_CONVERT;
        ret = yaml_alloc_convert(yamlfile);
    }
    if (!strcmp(arg, "output"))
    {
        yamlfile->state = YAML_ST_OUTPUT;
        ret = yaml_alloc_output(yamlfile);
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

    fdi = fopen(yamlfile->name, "rb");
    if( fdi == NULL )
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        LL_ERROR("Use --help option if needed.");
        return 1;
    }

    do
    {
        char *token;

        line = ymal_get_line(fdi);
        if (line == NULL)
        {
            break;
        }

        token = strtok(line, ":");

        LL_DEBUG("token: %s", token);

        if (token)
        {
            ret = yaml_get_command(yamlfile, token);
        }

        free(line);
    } while (ret == 0 && !feof(fdi));

    return ret;
}
