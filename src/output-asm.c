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

#include "output.h"
#include "tileset.h"
#include "strings.h"
#include "image.h"
#include "log.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

/*
 * Outputs to Assembly format.
 */
static int output_asm(unsigned char *arr, size_t size, FILE *fdo)
{
    size_t i;

    for (i = 0; i < size; ++i)
    {
        bool last = i + 1 == size;

        if (last || ((i + 1) % 32 == 0))
        {
            fprintf(fdo, "$%02x", arr[i]);
            if (!last)
            {
                fputs("\r\n\tdb\t", fdo);
            }
        }
        else
        {
             fprintf(fdo, "$%02x,", arr[i]);
        }
    }
    fputs("\r\n", fdo);

    return 0;
}

/*
 * Outputs a converted Assembly image.
 */
int output_asm_image(image_t *image)
{
    char *source = strdupcat(image->directory, ".asm");
    FILE *fds;

    LL_INFO(" - Writing \'%s\'", source);

    fds = fopen(source, "w");
    if (fds == NULL)
    {
        LL_ERROR(" Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fds, "%s_width := %d\r\n", image->name, image->width);
    fprintf(fds, "%s_height := %d\r\n", image->name, image->height);
    fprintf(fds, "%s_size := %d\r\n", image->name, image->size);
    fprintf(fds, "%s:\r\n\tdb\t", image->name);

    output_asm(image->data, image->size, fds);

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return 1;
}

/*
 * Outputs a converted Assembly tileset.
 */
int output_asm_tileset(tileset_t *tileset)
{
    char *source = strdupcat(tileset->directory, ".asm");
    FILE *fds;
    int i;

    LL_INFO(" - Writing \'%s\'", source);

    fds = fopen(source, "w");
    if (fds == NULL)
    {
        LL_ERROR(" Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fds, "%s_num_tiles := %d\r\n",
        tileset->image.name,
        tileset->numTiles);

    for (i = 0; i < tileset->numTiles; ++i)
    {
        tileset_tile_t *tile = &tileset->tiles[i];

        fprintf(fds, "%s_tile_%d:\r\n\tdb\t", tileset->image.name, i);

        output_asm(tile->data, tile->size, fds);
    }

    if (tileset->pTable == true)
    {
        fprintf(fds, "%s_tiles:\r\n",
            tileset->image.name);

        for (i = 0; i < tileset->numTiles; ++i)
        {
            fprintf(fds, "\tdl\t%s_tile_%d\r\n",
                tileset->image.name,
                i);
        }
    }

    free(source);

    return 0;

error:
    free(source);
    return 1;
}

/*
 * Outputs a converted Assembly tileset.
 */
int output_asm_palette(palette_t *palette)
{
    char *source = strdupcat(palette->directory, ".asm");
    FILE *fds;
    int i;

    LL_INFO(" - Writing \'%s\'", source);

    fds = fopen(source, "w");
    if (fds == NULL)
    {
        LL_ERROR(" Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fds, "sizeof_%s := %d\r\n", palette->name, palette->numEntries * 2);
    fprintf(fds, "%s:\r\n", palette->name);

    for (i = 0; i < palette->numEntries; ++i)
    {
        color_t *color = &palette->entries[i].color;

        fprintf(fds, "\tdw\t$%04x ; %3d: rgb(%3d, %3d, %3d)\r\n",
                color->target,
                i,
                color->rgb.r,
                color->rgb.g,
                color->rgb.b);
    }

    fclose(fds);

    free(source);

    return 0;

error:
    free(source);
    return 1;
}

/*
 * Outputs an include file for the output structure
 */
int output_asm_include_file(output_t *output)
{
    char *includeFile = output->includeFileName;
    char *includeName = strdup(output->includeFileName);
    char *tmp;
    FILE *fdi;
    int i, j, k;

    tmp = strchr(includeName, '.');
    if (tmp != NULL)
    {
        *tmp = '\0';
    }

    LL_INFO(" - Writing \'%s\'", includeFile);

    fdi = fopen(includeFile, "w");
    if (fdi == NULL)
    {
        LL_ERROR(" Could not open file: %s", strerror(errno));
        goto error;
    }

    for (i = 0; i < output->numPalettes; ++i)
    {
        fprintf(fdi, "include \'%s.asm\'\r\n", output->palettes[i]->name);
    }

    for (i = 0; i < output->numConverts; ++i)
    {
        convert_t *convert = output->converts[i];

        for (j = 0; j < convert->numImages; ++j)
        {
            image_t *image = &convert->images[j];

            fprintf(fdi, "include \'%s.asm\'\r\n", image->name);
        }

        for (j = 0; j < convert->numTilesetGroups; ++j)
        {
            tileset_group_t *tilesetGroup = convert->tilesetGroups[j];

            for (k = 0; k < tilesetGroup->numTilesets; ++k)
            {
                tileset_t *tileset = &tilesetGroup->tilesets[k];

                fprintf(fdi, "include \'%s.asm\'\r\n", tileset->image.name);
            }
        }
    }

    fclose(fdi);

    free(includeName);

    return 0;

error:
    free(includeName);
    return 1;
}
