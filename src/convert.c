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

#include "convert.h"
#include "strings.h"
#include "compress.h"
#include "log.h"

#include <string.h>
#include <glob.h>

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
    convert->compress = COMPRESS_NONE;
    convert->palette = NULL;
    convert->tilesetGroup = NULL;
    convert->style = CONVERT_STYLE_NORMAL;
    convert->numOmitIndices = 0;
    convert->widthAndHeight = true;
    convert->transparentIndex = -1;
    convert->bpp = BPP_8;
    convert->name = NULL;
    convert->paletteName = strdup("xlibc");
    convert->quantizeSpeed = CONVERT_DEFAULT_QUANTIZE_SPEED;
    convert->dither = 0;
    convert->rotate = 0;
    convert->flipx = false;
    convert->flipy = false;

    return convert;
}

/*
 * Adds a image file to a convert (does not load).
 */
static int convert_add_image(convert_t *convert, const char *path)
{
    image_t *image;

    if (convert == NULL || path == NULL)
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

    image->path = strdup(path);
    image->name = strings_basename(path);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->rlet = false;
    image->compressed = false;

    convert->numImages++;

    LL_DEBUG("Adding image: %s [%s]", image->path, image->name);

    return 0;
}

/*
 * Allocates output structure.
 */
tileset_group_t *convert_alloc_tileset_group(convert_t *convert)
{
    tileset_group_t *tmpTilesetGroup;

    LL_DEBUG("Allocating convert tileset group...");

    tmpTilesetGroup = tileset_group_alloc();
    if (tmpTilesetGroup == NULL)
    {
        LL_DEBUG("Memory error in %s", __func__);
        return NULL;
    }

    convert->tilesetGroup = tmpTilesetGroup;

    return tmpTilesetGroup;
}

/*
 * Adds a tileset to a convert (does not load).
 */
static int convert_add_tileset(convert_t *convert, const char *path)
{
    image_t *image;
    tileset_group_t *tilesetGroup;
    tileset_t *tileset;

    if (convert == NULL || path == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        return 1;
    }

    tilesetGroup = convert->tilesetGroup;

    tilesetGroup->tilesets =
        realloc(tilesetGroup->tilesets, (tilesetGroup->numTilesets + 1) * sizeof(tileset_t));
    if (tilesetGroup->tilesets == NULL)
    {
        LL_DEBUG("Memory error in %s", __func__);
        return 1;
    }

    tileset = &tilesetGroup->tilesets[tilesetGroup->numTilesets];

    tileset->tileHeight = tilesetGroup->tileHeight;
    tileset->tileWidth = tilesetGroup->tileWidth;
    tileset->pTable = tilesetGroup->pTable;
    tileset->tiles = NULL;
    tileset->numTiles = 0;

    image = &tileset->image;
    image->path = strdup(path);
    image->name = strings_basename(path);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->compressed = false;
    image->rlet = false;
    image->rotate = 0;
    image->flipx = false;
    image->flipy = false;

    tilesetGroup->numTilesets++;

    return 0;
}

/*
 * Adds a path which may or may not include images.
 */
int convert_add_image_path(convert_t *convert, const char *path)
{
    glob_t *globbuf = NULL;
    char **paths = NULL;
    int i;
    int len;

    if (convert == NULL || path == NULL)
    {
        return 1;
    }

    globbuf = strings_find_images(path);
    paths = globbuf->gl_pathv;
    len = globbuf->gl_pathc;

    if (len == 0)
    {
        LL_ERROR("Could not find file(s): \'%s\'", path);
        return 1;
    }

    for (i = 0; i < len; ++i)
    {
        convert_add_image(convert, paths[i]);
    }

    globfree(globbuf);
    free(globbuf);

    return 0;
}

/*
 * Adds a path which may or may not include images.
 */
int convert_add_tileset_path(convert_t *convert, const char *path)
{
    glob_t *globbuf = NULL;
    char **paths = NULL;
    int i;
    int len;

    if (convert == NULL || path == NULL)
    {
        return 1;
    }

    globbuf = strings_find_images(path);
    paths = globbuf->gl_pathv;
    len = globbuf->gl_pathc;

    if (len == 0)
    {
        LL_ERROR("Could not find file(s): \'%s\'", path);
        return 1;
    }

    for (i = 0; i < len; ++i)
    {
        convert_add_tileset(convert, paths[i]);
    }

    globfree(globbuf);
    free(globbuf);

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

    tileset_group_free(convert->tilesetGroup);
    free(convert->tilesetGroup);
    convert->tilesetGroup = NULL;

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

    if (convert == NULL || palettes == NULL || convert->paletteName == NULL)
    {
        LL_ERROR("A palette with the same name \'%s\' already exists.",
            convert->name);
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

    LL_ERROR("No palette \'%s\' found for convert \'%s\'",
        convert->paletteName,
        convert->name);
    return 1;
}

/*
 * Converts an image using flags.
 */
static int convert_image(convert_t *convert, image_t *image)
{
    int ret;

    if (convert->style == CONVERT_STYLE_RLET)
    {
        ret = image_rlet(image, convert->transparentIndex);
        if (ret != 0)
        {
            return ret;
        }

        image->rlet = true;
    }

    if (convert->numOmitIndices != 0)
    {
        ret = image_remove_omits(image, convert->omitIndices, convert->numOmitIndices);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (convert->bpp != BPP_8)
    {
        ret = image_set_bpp(image, convert->bpp, convert->palette->numEntries);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (convert->widthAndHeight == true)
    {
        ret = image_add_width_and_height(image);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (convert->compress != COMPRESS_NONE)
    {
        ret = image_compress(image, convert->compress);
        if (ret != 0)
        {
            return ret;
        }

        image->compressed = true;
    }

    return 0;
}

/*
 * Converts a tileset to multiple data blocks for conversion.
 */
int convert_tileset(convert_t *convert, tileset_t *tileset)
{
    int ret = 0;
    int i, j, k;
    int x, y;

    tileset->numTiles =
        (tileset->image.width / tileset->tileWidth) *
        (tileset->image.height / tileset->tileHeight);

    if (tileset->image.width % tileset->tileWidth)
    {
        LL_ERROR("Image dimensions do not support tile width");
        return 1;
    }
    if (tileset->image.height % tileset->tileHeight)
    {
        LL_ERROR("Image dimensions do not support tile height");
        return 1;
    }

    tileset->compressed = convert->compress != COMPRESS_NONE;

    tileset_alloc_tiles(tileset);

    y = x = 0;

    for (i = 0; i < tileset->numTiles; ++i)
    {
        image_t tile =
        {
            .data = tileset->tiles[i].data,
            .width = tileset->tileWidth,
            .height = tileset->tileHeight,
            .size = tileset->tiles[i].size,
            .name = NULL,
            .path = NULL
        };
        int byte = 0;

        if (ret != 0)
        {
            break;
        }

        for (j = 0; j < tile.height; ++j)
        {
            int offset = j * tileset->image.width + y;
            for (k = 0; k < tile.width; ++k)
            {
                tile.data[byte] = tileset->image.data[k + x + offset];
                byte++;
            }
        }

        ret = convert_image(convert, &tile);
        if (ret != 0)
        {
            break;
        }

        tileset->tiles[i].size = tile.size;
        tileset->tiles[i].data = tile.data;

        x += tileset->tileWidth;

        if (x >= tileset->image.width)
        {
            x = 0;
            y += tileset->image.width * tileset->tileHeight;
        }
    }

    return ret;
}

/*
 * Converts an image to a palette or raw data as needed.
 */
int convert_convert(convert_t *convert, palette_t **palettes, int numPalettes)
{
    int i, j;
    int ret = 0;

    if (convert == NULL)
    {
        return 1;
    }

    if (convert->numImages > 0)
    {
        LL_INFO("Generating convert \'%s\'", convert->name);
    }

    ret = convert_find_palette(convert, palettes, numPalettes);
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; i < convert->numImages; ++i)
    {
        image_t *image = &convert->images[i];

        if (ret != 0)
        {
            break;
        }

        LL_INFO(" - Reading image \'%s\'",
            image->path);

        image->dither = convert->dither;
        image->rotate = convert->rotate;
        image->flipx = convert->flipx;
        image->flipy = convert->flipy;
        image->quantizeSpeed = convert->quantizeSpeed;

        ret = image_load(image);
        if (ret != 0)
        {
            LL_ERROR("Failed to load image \'%s\'", image->path);
            break;
        }

        ret = image_quantize(image, convert->palette);
        if (ret != 0)
        {
            break;
        }

        ret = convert_image(convert, image);
        if (ret != 0)
        {
            break;
        }
    }

    if (ret == 0 && convert->tilesetGroup != NULL)
    {
        tileset_group_t *tilesetGroup = convert->tilesetGroup;

        LL_INFO("Converting tilesets for \'%s\'", convert->name);

        for (j = 0; j < tilesetGroup->numTilesets; ++j)
        {
            tileset_t *tileset = &tilesetGroup->tilesets[j];
            image_t *image = &tileset->image;

            if (ret != 0)
            {
                break;
            }

            LL_INFO(" - Reading tileset \'%s\'",
                image->path);

            ret = image_load(image);
            if (ret != 0)
            {
                LL_ERROR("Failed to load image \'%s\'", image->path);
                break;
            }

            image->dither = convert->dither;
            image->quantizeSpeed = convert->quantizeSpeed;

            ret = image_quantize(image, convert->palette);
            if (ret != 0)
            {
                break;
            }

            ret = convert_tileset(convert, tileset);
            if (ret != 0)
            {
                break;
            }
        }
    }

    return ret;
}
