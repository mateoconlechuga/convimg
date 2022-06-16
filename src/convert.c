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

#include "convert.h"
#include "strings.h"
#include "compress.h"
#include "tileset.h"
#include "log.h"

#include <string.h>
#include <glob.h>

struct convert *convert_alloc(void)
{
    struct convert *convert = malloc(sizeof(struct convert));
    if (convert == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return NULL;
    }

    convert->images = NULL;
    convert->nr_images = 0;
    convert->compress = COMPRESS_NONE;
    convert->palette = NULL;
    convert->palette_offset = 0;
    convert->tileset_group = NULL;
    convert->style = CONVERT_STYLE_NORMAL;
    convert->nr_omit_indices = 0;
    convert->add_width_height = true;
    convert->transparent_index = -1;
    convert->bpp = BPP_8;
    convert->name = NULL;
    convert->palette_name = strdup("xlibc");
    convert->quantize_speed = CONVERT_DEFAULT_QUANTIZE_SPEED;
    convert->dither = 0;
    convert->rotate = 0;
    convert->flip_x = false;
    convert->flip_y = false;

    return convert;
}

static int convert_add_image(struct convert *convert, const char *path)
{
    struct image *image;

    if (convert == NULL || path == NULL)
    {
        return -1;
    }

    convert->images =
        realloc(convert->images, (convert->nr_images + 1) * sizeof(struct image));
    if (convert->images == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    image = &convert->images[convert->nr_images];

    image->path = strdup(path);
    image->name = strings_basename(path);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->rlet = false;
    image->compressed = false;

    convert->nr_images++;

    LOG_DEBUG("Adding convert image: %s [%s]\n", image->path, image->name);

    return 0;
}

struct tileset_group *convert_alloc_tileset_group(struct convert *convert)
{
    struct tileset_group *tmp;

    LOG_DEBUG("Allocating convert tileset group...\n");

    tmp = tileset_group_alloc();
    if (tmp == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return NULL;
    }

    convert->tileset_group = tmp;

    return tmp;
}

static int convert_add_tileset(struct convert *convert, const char *path)
{
    struct image *image;
    struct tileset_group *tileset_group;
    struct tileset *tileset;

    if (convert == NULL || path == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    tileset_group = convert->tileset_group;

    tileset_group->tilesets =
        realloc(tileset_group->tilesets, (tileset_group->nr_tilesets + 1) * sizeof(struct tileset));
    if (tileset_group->tilesets == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    tileset = &tileset_group->tilesets[tileset_group->nr_tilesets];

    tileset->tile_height = tileset_group->tile_height;
    tileset->tile_width = tileset_group->tile_width;
    tileset->p_table = tileset_group->p_table;
    tileset->tiles = NULL;
    tileset->nr_tiles = 0;

    image = &tileset->image;
    image->path = strdup(path);
    image->name = strings_basename(path);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->compressed = false;
    image->rlet = false;
    image->rotate = 0;
    image->flip_x = false;
    image->flip_y = false;

    tileset_group->nr_tilesets++;

    return 0;
}

int convert_add_image_path(struct convert *convert, const char *path)
{
    static glob_t globbuf;
    char **paths = NULL;
    char *realPath;
    int i;
    int len;

    if (convert == NULL || path == NULL)
    {
        return -1;
    }

    realPath = strings_find_images(path, &globbuf);
    if (realPath == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    paths = globbuf.gl_pathv;
    len = globbuf.gl_pathc;

    if (len == 0)
    {
        LOG_ERROR("Could not find file(s): \'%s\'\n", realPath);
        globfree(&globbuf);
        free(realPath);
        return -1;
    }

    for (i = 0; i < len; ++i)
    {
        convert_add_image(convert, paths[i]);
    }

    globfree(&globbuf);
    free(realPath);

    return 0;
}

int convert_add_tileset_path(struct convert *convert, const char *path)
{
    static glob_t globbuf;
    char **paths = NULL;
    char *realPath;
    int i;
    int len;

    if (convert == NULL || path == NULL)
    {
        return -1;
    }

    realPath = strings_find_images(path, &globbuf);
    if (realPath == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    paths = globbuf.gl_pathv;
    len = globbuf.gl_pathc;

    if (len == 0)
    {
        LOG_ERROR("Could not find file(s): \'%s\'\n", realPath);
        globfree(&globbuf);
        free(realPath);
        return -1;
    }

    for (i = 0; i < len; ++i)
    {
        convert_add_tileset(convert, paths[i]);
    }

    globfree(&globbuf);
    free(realPath);

    return 0;
}

void convert_free(struct convert *convert)
{
    int i;

    if (convert == NULL)
    {
        return;
    }

    for (i = 0; i < convert->nr_images; ++i)
    {
        image_free(&convert->images[i]);
    }

    tileset_group_free(convert->tileset_group);
    free(convert->tileset_group);
    convert->tileset_group = NULL;

    free(convert->images);
    convert->images = NULL;

    free(convert->name);
    convert->name = NULL;

    free(convert->palette_name);
    convert->palette_name = NULL;
}

int convert_find_palette(struct convert *convert, struct palette **palettes, int nr_palettes)
{
    int i;

    if (convert == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    if (palettes == NULL || convert->palette_name == NULL)
    {
        goto error;
    }

    for (i = 0; i < nr_palettes; ++i)
    {
        if (!strcmp(convert->palette_name, palettes[i]->name))
        {
            convert->palette = palettes[i];
            return 0;
        }
    }

error:
    LOG_ERROR("No palette \'%s\' found for convert \'%s\'\n",
        convert->palette_name,
        convert->name);

    return -1;
}

static int convert_image(struct convert *convert, struct image *image)
{
    int ret;

    if (convert->palette_offset != 0)
    {
        if (convert->palette_offset + convert->palette->nr_entries >=
            PALETTE_MAX_ENTRIES)
        {
            LOG_ERROR("Palette offset places indices out of range for "
                      "convert \'%s\'\n",
                convert->name);
            return -1;
        }
        ret = image_add_offset(image, convert->palette_offset);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (convert->style == CONVERT_STYLE_RLET)
    {
        ret = image_rlet(image, convert->transparent_index);
        if (ret != 0)
        {
            return ret;
        }

        image->rlet = true;
    }

    if (convert->nr_omit_indices != 0)
    {
        ret = image_remove_omits(image, convert->omit_indices, convert->nr_omit_indices);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (convert->bpp != BPP_8)
    {
        ret = image_set_bpp(image, convert->bpp, convert->palette->nr_entries);
        if (ret != 0)
        {
            return ret;
        }
    }

    if (convert->add_width_height == true)
    {
        ret = image_add_width_and_height(image);
        if (ret != 0)
        {
            return ret;
        }
    }

    image->orig_size = image->size;

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

int convert_tileset(struct convert *convert, struct tileset *tileset)
{
    int i, j, k;
    int x, y;

    tileset->nr_tiles =
        (tileset->image.width / tileset->tile_width) *
        (tileset->image.height / tileset->tile_height);

    if (tileset->image.width % tileset->tile_width)
    {
        LOG_ERROR("Image dimensions do not support tile width.\n");
        return -1;
    }
    if (tileset->image.height % tileset->tile_height)
    {
        LOG_ERROR("Image dimensions do not support tile height.\n");
        return -1;
    }

    tileset->compressed = convert->compress != COMPRESS_NONE;
    tileset->rlet = convert->style == CONVERT_STYLE_RLET;

    tileset_alloc_tiles(tileset);

    y = x = 0;

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        struct image tile =
        {
            .data = tileset->tiles[i].data,
            .width = tileset->tile_width,
            .height = tileset->tile_height,
            .size = tileset->tiles[i].size,
            .name = NULL,
            .path = NULL
        };
        int byte = 0;
        int ret;

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
            return ret;
        }

        tileset->tiles[i].size = tile.size;
        tileset->tiles[i].data = tile.data;

        x += tileset->tile_width;

        if (x >= tileset->image.width)
        {
            x = 0;
            y += tileset->image.width * tileset->tile_height;
        }
    }

    return 0;
}

int convert_convert(struct convert *convert, struct palette **palettes, int nr_palettes)
{
    int ret;
    int i;

    if (convert == NULL)
    {
        return -1;
    }

    if (convert->nr_images > 0)
    {
        LOG_INFO("Generating convert \'%s\'\n", convert->name);
    }

    ret = convert_find_palette(convert, palettes, nr_palettes);
    if (ret != 0)
    {
        return -1;
    }

    for (i = 0; i < convert->nr_images; ++i)
    {
        struct image *image = &convert->images[i];

        LOG_INFO(" - Reading image \'%s\'\n",
            image->path);

        image->dither = convert->dither;
        image->rotate = convert->rotate;
        image->flip_x = convert->flip_x;
        image->flip_y = convert->flip_y;
        image->transparent_index = convert->transparent_index;
        image->quantize_speed = convert->quantize_speed;

        ret = image_load(image);
        if (ret != 0)
        {
            LOG_ERROR("Failed to load image \'%s\'\n", image->path);
            return -1;
        }

        if (convert->add_width_height == true)
        {
            if (image->width > 255)
            {
                LOG_ERROR("Image \'%s\' width is %u. Maximum width is 255.\n",
                    image->path,
                    image->width);
                return -1;
            }

            if (image->height > 255)
            {
                LOG_ERROR("Image \'%s\' height is %u. Maximum height is 255.\n",
                    image->path,
                    image->width);
                return -1;
            }
        }

        ret = image_quantize(image, convert->palette);
        if (ret != 0)
        {
            return -1;
        }

        if (image->bad_alpha)
        {
            LOG_WARNING("Image has pixels with an alpha not 0 or 255.\n");
            LOG_WARNING("This may result in incorrect color conversion.\n");
        }

        ret = convert_image(convert, image);
        if (ret != 0)
        {
            return -1;
        }
    }

    if (convert->tileset_group != NULL)
    {
        struct tileset_group *tileset_group = convert->tileset_group;
        int j;

        LOG_INFO("Converting tilesets for \'%s\'\n", convert->name);

        for (j = 0; j < tileset_group->nr_tilesets; ++j)
        {
            struct tileset *tileset = &tileset_group->tilesets[j];
            struct image *image = &tileset->image;

            LOG_INFO(" - Reading tileset \'%s\'\n",
                image->path);

            ret = image_load(image);
            if (ret != 0)
            {
                LOG_ERROR("Failed to load image \'%s\'\n", image->path);
                return -1;
            }

            image->dither = convert->dither;
            image->quantize_speed = convert->quantize_speed;

            ret = image_quantize(image, convert->palette);
            if (ret != 0)
            {
                return -1;
            }

            if (image->bad_alpha)
            {
                LOG_WARNING("Tileset has pixels with an alpha not 0 or 255.\n");
                LOG_WARNING("This may result in incorrect color conversion.\n");
            }

            ret = convert_tileset(convert, tileset);
            if (ret != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}
