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
#include "tileset.h"
#include "strings.h"
#include "image.h"
#include "log.h"

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

/*
 * Outputs to C format.
 */
static int output_c(unsigned char *arr, size_t size, FILE *fdo)
{
    size_t i;

    for (i = 0; i < size; ++i)
    {
        if (i % 32 == 0)
        {
            fputs("\n    ", fdo);
        }

        if (i + 1 == size)
        {
            fprintf(fdo, "0x%02x", arr[i]);
        }
        else
        {
            fprintf(fdo, "0x%02x,", arr[i]);
        }
    }
    fprintf(fdo, "\n};\n");

    return 0;
}

/*
 * Outputs a converted C image.
 */
int output_c_image(image_t *image)
{
    char *header = strdupcat(image->directory, ".h");
    char *source = strdupcat(image->directory, ".c");
    FILE *fdh;
    FILE *fds;

    LL_INFO(" - Writing \'%s\'", header);

    fdh = fopen(header, "wt");
    if (fdh == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fdh, "#ifndef %s_include_file\n", image->name);
    fprintf(fdh, "#define %s_include_file\n", image->name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#define %s_width %d\n", image->name, image->width);
    fprintf(fdh, "#define %s_height %d\n", image->name, image->height);
    fprintf(fdh, "#define %s_size %d\n", image->name, image->origSize);

    if (image->compressed)
    {
        fprintf(fdh, "#define %s_compressed_size %d\n", image->name, image->size);
        fprintf(fdh, "extern unsigned char %s_compressed[%d];\n", image->name, image->size);
    }
    else
    {
        fprintf(fdh, "#define %s ((%s*)%s_data)\n",
            image->name,
            image->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
            image->name);
        fprintf(fdh, "extern unsigned char %s_data[%d];\n", image->name, image->size);
    }

    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");

    fclose(fdh);

    LL_INFO(" - Writing \'%s\'", source);

    fds = fopen(source, "wt");
    if (fds == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    if (image->compressed)
    {
        fprintf(fds, "unsigned char %s_compressed[%d] =\n{", image->name, image->size);
    }
    else
    {
        fprintf(fds, "unsigned char %s_data[%d] =\n{", image->name, image->size);
    }

    output_c(image->data, image->size, fds);

    fclose(fds);

    free(header);
    free(source);

    return 0;

error:
    free(header);
    free(source);
    return 1;
}


/*
 * Outputs a converted C tileset.
 */
int output_c_tileset(tileset_t *tileset)
{
    char *header = strdupcat(tileset->directory, ".h");
    char *source = strdupcat(tileset->directory, ".c");
    FILE *fdh;
    FILE *fds;
    int i;

    LL_INFO(" - Writing \'%s\'", header);

    fdh = fopen(header, "wt");
    if (fdh == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fdh, "#ifndef %s_include_file\n", tileset->image.name);
    fprintf(fdh, "#define %s_include_file\n", tileset->image.name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");

    for (i = 0; i < tileset->numTiles; ++i)
    {
        tileset_tile_t *tile = &tileset->tiles[i];

        if (tileset->compressed)
        {
            fprintf(fdh, "extern unsigned char %s_tile_%d_compressed[%d];\n",
                tileset->image.name,
                i,
                tile->size);
        }
        else
        {
            fprintf(fdh, "extern unsigned char %s_tile_%d_data[%d];\n",
                tileset->image.name,
                i,
                tile->size);
            fprintf(fdh, "#define %s_tile_%d ((%s*)%s_tile_%d_data)\n",
                tileset->image.name,
                i,
                tileset->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                tileset->image.name,
                i);
        }
    }

    fprintf(fdh, "#define %s_num_tiles %d\n",
        tileset->image.name,
        tileset->numTiles);

    if (tileset->pTable)
    {
        if (tileset->compressed)
        {
            fprintf(fdh, "extern unsigned char *%s_tiles_compressed[%d];\n",
                tileset->image.name,
                tileset->numTiles);
        }
        else
        {
            fprintf(fdh, "extern unsigned char *%s_tiles_data[%d];\n",
                tileset->image.name,
                tileset->numTiles);

            fprintf(fdh, "#define %s_tiles ((%s**)%s_tiles_data)\n",
                tileset->image.name,
                tileset->rlet ? "gfx_rletsprite_t" : "gfx_sprite_t",
                tileset->image.name);
        }
    }

    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");

    fclose(fdh);

    LL_INFO(" - Writing \'%s\'", source);

    fds = fopen(source, "wt");
    if (fds == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    for (i = 0; i < tileset->numTiles; ++i)
    {
        tileset_tile_t *tile = &tileset->tiles[i];

        if (tileset->compressed)
        {
            fprintf(fds, "unsigned char %s_tile_%d_compressed[%d] =\n{",
                tileset->image.name,
                i,
                tile->size);
        }
        else
        {
            fprintf(fds, "unsigned char %s_tile_%d_data[%d] =\n{",
                tileset->image.name,
                i,
                tile->size);
        }

        output_c(tile->data, tile->size, fds);
    }

    if (tileset->pTable)
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

        for (i = 0; i < tileset->numTiles; ++i)
        {
            if (tileset->compressed)
            {
                fprintf(fds, "    %s_tile_%d_compressed,\n",
                    tileset->image.name,
                    i);
            }
            else
            {
                fprintf(fds, "    %s_tile_%d_data,\n",
                    tileset->image.name,
                    i);
            }
        }

        fprintf(fds, "};\n");
    }

    fclose(fds);

    free(header);
    free(source);

    return 0;

error:
    free(header);
    free(source);
    return 1;
}

/*
 * Outputs a converted C tileset.
 */
int output_c_palette(palette_t *palette)
{
    char *header = strdupcat(palette->directory, ".h");
    char *source = strdupcat(palette->directory, ".c");
    int size = palette->numEntries * 2;
    FILE *fdh;
    FILE *fds;
    int i;

    LL_INFO(" - Writing \'%s\'", header);

    fdh = fopen(header, "wt");
    if (fdh == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fdh, "#ifndef %s_include_file\n", palette->name);
    fprintf(fdh, "#define %s_include_file\n", palette->name);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "extern \"C\" {\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#define sizeof_%s %d\n", palette->name, size);
    fprintf(fdh, "extern unsigned char %s[%d];\n", palette->name, size);
    fprintf(fdh, "\n");
    fprintf(fdh, "#ifdef __cplusplus\n");
    fprintf(fdh, "}\n");
    fprintf(fdh, "#endif\n");
    fprintf(fdh, "\n");
    fprintf(fdh, "#endif\n");

    fclose(fdh);

    LL_INFO(" - Writing \'%s\'", source);

    fds = fopen(source, "wt");
    if (fds == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fds, "unsigned char %s[%d] =\n{\n", palette->name, size);

    for (i = 0; i < palette->numEntries; ++i)
    {
        color_t *color = &palette->entries[i].color;
        color_t *origcolor = &palette->entries[i].origcolor;

        if (palette->entries[i].exact)
        {
            fprintf(fds, "    0x%02x, 0x%02x, /* %3d: rgb(%3d, %3d, %3d) [exact original: rgb(%3d, %3d, %3d)] */\n",
                    color->target & 255,
                    (color->target >> 8) & 255,
                    i,
                    color->rgb.r,
                    color->rgb.g,
                    color->rgb.b,
                    origcolor->rgb.r,
                    origcolor->rgb.g,
                    origcolor->rgb.b);
        }
        else if (!palette->entries[i].valid)
        {
            fprintf(fds, "    0x00, 0x00, /* %3d: (unused) */\n", i);
        }
        else
        {
            fprintf(fds, "    0x%02x, 0x%02x, /* %3d: rgb(%3d, %3d, %3d) */\n",
                    color->target & 255,
                    (color->target >> 8) & 255,
                    i,
                    color->rgb.r,
                    color->rgb.g,
                    color->rgb.b);
        }
    }
    fprintf(fds, "};\n");

    fclose(fds);

    free(header);
    free(source);

    return 0;

error:
    free(header);
    free(source);
    return 1;
}

/*
 * Outputs an include file for the output structure
 */
int output_c_include_file(output_t *output)
{
    char *includeFile = strdupcat(output->directory, output->includeFileName);
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

    fdi = fopen(includeFile, "wt");
    if (fdi == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        goto error;
    }

    fprintf(fdi, "#ifndef %s_include_file\n", includeName);
    fprintf(fdi, "#define %s_include_file\n", includeName);
    fprintf(fdi, "\n");
    fprintf(fdi, "#ifdef __cplusplus\n");
    fprintf(fdi, "extern \"C\" {\n");
    fprintf(fdi, "#endif\n");
    fprintf(fdi, "\n");

    for (i = 0; i < output->numPalettes; ++i)
    {
        fprintf(fdi, "#include \"%s.h\"\n", output->palettes[i]->name);
    }

    for (i = 0; i < output->numConverts; ++i)
    {
        convert_t *convert = output->converts[i];
        tileset_group_t *tilesetGroup = convert->tilesetGroup;

        fprintf(fdi, "#define %s_palette_offset %d\n",
            convert->name, convert->paletteOffset);

        for (j = 0; j < convert->numImages; ++j)
        {
            image_t *image = &convert->images[j];

            fprintf(fdi, "#include \"%s.h\"\n", image->name);
        }

        if (tilesetGroup != NULL)
        {
            for (k = 0; k < tilesetGroup->numTilesets; ++k)
            {
                tileset_t *tileset = &tilesetGroup->tilesets[k];

                fprintf(fdi, "#include \"%s.h\"\n", tileset->image.name);
            }
        }
    }

    fprintf(fdi, "\n");
    fprintf(fdi, "#ifdef __cplusplus\n");
    fprintf(fdi, "}\n");
    fprintf(fdi, "#endif\n");
    fprintf(fdi, "\n");
    fprintf(fdi, "#endif\n");

    fclose(fdi);

    free(includeName);
    free(includeFile);

    return 0;

error:
    free(includeName);
    free(includeFile);
    return 1;
}
