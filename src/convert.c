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

#include "convert.h"
#include "compress.h"
#include "log.h"

#include <string.h>

/*
 * Allocates a convert structure.
 */
convert_t *convert_alloc(void)
{
    convert_t *convert = NULL;

    convert = malloc(sizeof(convert_t));
    if (convert == NULL)
    {
        return NULL;
    }

    convert->images = NULL;
    convert->numImages = 0;
    convert->compression = COMPRESS_NONE;
    convert->palette = NULL;
    convert->tileset.enabled = false;
    convert->tileset.tileWidth = 0;
    convert->tileset.tileHeight = 0;
    convert->tileset.pTable = true;
    convert->tileset.tiles = NULL;
    convert->style = CONVERT_STYLE_NORMAL;
    convert->numOmitIndices = 0;
    convert->widthAndHeight = true;
    convert->bpp = BPP_8;
    convert->name = NULL;
    convert->paletteName = "xlibc";

    return convert;
}

/*
 * Adds a image file to a convert (does not load).
 */
int convert_add_image(convert_t *convert, const char *name)
{
    image_t *image;

    if (convert == NULL || name == NULL)
    {
        return 1;
    }

    convert->images =
        realloc(convert->images, (convert->numImages + 1) * sizeof(image_t));
    if (convert->images == NULL)
    {
        return 1;
    }

    image = &convert->images[convert->numImages];

    image->name = strdup(name);
    image->data = NULL;
    image->width = 0;
    image->height = 0;

    convert->numImages++;

    return 0;
}

/*
 * Frees an allocated convert.
 */
void convert_free(convert_t *convert)
{
    int i;

    if (convert == NULL)
    {
        return;
    }

    for (i = 0; i < convert->numImages; ++i)
    {
        image_free(&convert->images[i]);
    }

    if (convert->tileset.enabled)
    {
        tileset_free(&convert->tileset);
    }

    free(convert->images);
    convert->images = NULL;

    free(convert->name);
    convert->name = NULL;

    free(convert->paletteName);
    convert->paletteName = NULL;
}

/*
 * Gets the pallete data used for converting.
 */
int convert_find_palette(convert_t *convert, palette_t **palettes, int numPalettes)
{
    int i;

    if (convert == NULL || palettes == NULL)
    {
        return 1;
    }

    for (i = 0; i < numPalettes; ++i)
    {
        if (!strcmp(convert->paletteName, palettes[i]->name))
        {
            convert->palette = palettes[i];
            return 0;
        }
    }

    LL_ERROR("No matching palette name found to convert \'%s\'",
        convert->name);
    return 1;
}

/*
 * Removes omited indicies from the converted data.
 * Reallocs array as needed.
 */
int convert_remove_omits(convert_t *convert, uint8_t **data, int *size)
{
    int i, j;
    int newSize = 0;
    uint8_t *newData;

    if (convert->numOmitIndices == 0)
    {
        return 0;
    }

    newData = malloc(*size);
    if (newData == NULL)
    {
        return 1;
    }

    for (i = 0; i < *size; ++i)
    {
        for (j = 0; j < convert->numOmitIndices; ++j)
        {
            if ((*data)[i] == convert->omitIndices[j])
            {
                goto nextbyte;
            }
        }

        newData[newSize] = (*data)[i];
        newSize++;

nextbyte:
        continue;
    }

    free(*data);
    *data = newData;
    *size = newSize;

    return 0;
}

/*
 * Converts a tileset to multiple data blocks for conversion.
 */
int convert_tileset(convert_t *convert, tileset_t *tileset, image_t *image)
{
    int ret = 0;
    int i, j, k;
    int y;
    int x;
    int byte;

    tileset->numTiles =
        (image->width / tileset->tileWidth) *
        (image->height / tileset->tileHeight);

    if (image->width % tileset->tileWidth)
    {
        LL_ERROR("Image dimensions do not support tile width");
        return 1;
    }
    if (image->height % tileset->tileHeight)
    {
        LL_ERROR("Image dimensions do not support tile height");
        return 1;
    }

    tileset_alloc_tiles(tileset);

    y = x = 0;

    for (i = 0; i < tileset->numTiles; ++i)
    {
        uint8_t *data = tileset->tiles[i].data;
        byte = 0;

        for (j = 0; j < tileset->tileHeight; j++)
        {
            int offset = j * image->width + y;
            for (k = 0; k < tileset->tileWidth; k++)
            {
                data[byte] = image->data[k + x + offset];
                byte++;
            }
        }

        ret = convert_remove_omits(convert, &data, &tileset->tiles[i].size);
        if (ret != 0)
        {
            break;
        }

        x += tileset->tileWidth;

        if (x >= image->width)
        {
            x = 0;
            y += image->width * tileset->tileHeight;
        }
    }

    return ret;
}

/*
 * Converts an image to a palette or raw data as needed.
 */
int convert_images(convert_t *convert, palette_t **palettes, int numPalettes)
{
    int i;
    int ret = 0;

    if (convert == NULL)
    {
        return 1;
    }

    LL_INFO("Converting images \'%s\'", convert->name);

    ret = convert_find_palette(convert, palettes, numPalettes);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < convert->numImages; ++i)
    {
        image_t *image = &convert->images[i];

        LL_INFO(" Reading \'%s\' (%d of %d)",
            image->name,
            i + 1,
            convert->numImages);

        ret = image_load(image);
        if (ret != 0)
        {
            LL_ERROR("Failed to load image %s", image->name);
            break;
        }

        ret = image_quantize(image, convert->palette);
        if (ret != 0)
        {
            break;
        }

        if (convert->tileset.enabled)
        {
            ret = convert_tileset(convert, &convert->tileset, image);
            if (ret != 0)
            {
                break;
            }
        }
        else
        {
            ret = convert_remove_omits(convert, &image->data, &image->size);
            if (ret != 0)
            {
                break;
            }
        }
    }

    return ret;
}
