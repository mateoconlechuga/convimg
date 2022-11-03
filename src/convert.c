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
        return NULL;
    }

    convert->images = NULL;
    convert->nr_images = 0;
    convert->compress = COMPRESS_NONE;
    convert->palette = NULL;
    convert->palette_offset = 0;
    convert->tileset_group = NULL;
    convert->style = CONVERT_STYLE_PALETTE;
    convert->nr_omit_indices = 0;
    convert->add_width_height = true;
    convert->transparent_index = 0;
    convert->bpp = BPP_8;
    convert->name = NULL;
    convert->palette_name = NULL;
    convert->quantize_speed = CONVERT_DEFAULT_QUANTIZE_SPEED;
    convert->dither = 0;
    convert->rotate = 0;
    convert->flip_x = false;
    convert->flip_y = false;

    return convert;
}

static bool convert_is_palette_style(const struct convert *convert)
{
    return convert->style == CONVERT_STYLE_PALETTE || convert->style == CONVERT_STYLE_RLET;
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
        return -1;
    }

    image = &convert->images[convert->nr_images];

    image_init(image, path);

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
    uint32_t i;

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
    if (convert_is_palette_style(convert))
    {
        if (image_quantize(image, convert->palette))
        {
            return -1;
        }

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

            if (image_add_offset(image, convert->palette_offset))
            {
                return -1;
            }
        }

        if (convert->style == CONVERT_STYLE_RLET)
        {
            if (image_rlet(image, convert->transparent_index))
            {
                return -1;
            }
        }

        if (convert->nr_omit_indices)
        {
            if (image_remove_omits(image, convert->omit_indices, convert->nr_omit_indices))
            {
                return -1;
            }
        }

        if (convert->bpp != BPP_8)
        {
            if (image_set_bpp(image, convert->bpp, convert->palette->nr_entries))
            {
                return -1;
            }
        }
    }
    else
    {
        color_format_t fmt;

        switch (convert->style)
        {
            case CONVERT_STYLE_RGB565:
                fmt = COLOR_565_RGB;
                break;

            case CONVERT_STYLE_BGR565:
                fmt = COLOR_565_BGR;
                break;

            case CONVERT_STYLE_GBGR1555:
                fmt = COLOR_1555_GBGR;
                break;

            default:
                return -1;
        }

        if (image_colorspace_convert(image, fmt))
        {
            return -1;
        }
    }

    if (convert->add_width_height == true)
    {
        if (image_add_width_and_height(image))
        {
            return -1;
        }
    }

    image->uncompressed_size = image->data_size;
    
    if (convert->compress != COMPRESS_NONE)
    {
        if (image_compress(image, convert->compress))
        {
            return -1;
        }

        image->compressed = true;
    }

    return 0;
}

int convert_tileset(struct convert *convert, struct tileset *tileset)
{
    uint32_t i;
    uint32_t x;
    uint32_t y;

    tileset->rlet = convert->style == CONVERT_STYLE_RLET;
    tileset->compressed = convert->compress != COMPRESS_NONE;

    y = 0;
    x = 0;

    for (i = 0; i < tileset->nr_tiles; ++i)
    {
        uint32_t tile_dim = tileset->tile_width * tileset->tile_height;
        uint32_t tile_data_size = tile_dim * sizeof(uint32_t);
        uint32_t tile_stride = tileset->tile_width * sizeof(uint32_t);
        uint32_t image_stride = tileset->image.width * sizeof(uint32_t);
        uint8_t *tile_data;
        uint8_t *dst;
        uint32_t j;

        tile_data = malloc(tile_data_size);
        if (tile_data == NULL)
        {
            return -1;
        }

        struct image tile =
        {
            .data = tile_data,
            .data_size = tile_data_size,
            .width = tileset->tile_width,
            .height = tileset->tile_height,
            .name = NULL,
            .path = NULL,
        };

        dst = tile_data;

        for (j = 0; j < tile.height; ++j)
        {
            uint32_t o = (j * image_stride) + y;

            memcpy(dst, &tileset->image.data[x + o], tile_stride);

            dst += tile_stride;
        }

        x += tile_stride;

        if (x >= image_stride)
        {
            x = 0;
            y += tileset->tile_height * image_stride;
        }

        if (convert_image(convert, &tile))
        {
            return -1;
        }

        tileset->tiles[i].data_size = tile.data_size;
        tileset->tiles[i].data = tile.data;
    }

    return 0;
}

int convert_convert(struct convert *convert, struct palette **palettes, int nr_palettes)
{
    uint32_t i;

    if (convert == NULL)
    {
        return -1;
    }

    if (convert->nr_images > 0 || convert->tileset_group != NULL)
    {
        LOG_INFO("Generating convert \'%s\'\n", convert->name);
    }

    if (convert_is_palette_style(convert))
    {
        if (convert_find_palette(convert, palettes, nr_palettes))
        {
            return -1;
        }
    }

    for (i = 0; i < convert->nr_images; ++i)
    {
        struct image *image = &convert->images[i];
        
        /* assign values to image based on convert flags */
        image->quantize_speed = convert->quantize_speed;
        image->dither = convert->dither;
        image->rotate = convert->rotate;
        image->flip_x = convert->flip_x;
        image->flip_y = convert->flip_y;
        image->transparent_index = convert->transparent_index;
        image->rlet = convert->style == CONVERT_STYLE_RLET;

        image->gfx = false;
        if ((image->rlet || convert->add_width_height) && convert->bpp == BPP_8)
        {
            image->gfx = true;
        }

        LOG_INFO(" - Reading image \'%s\'\n",
                 image->path);

        if (image_load(image))
        {
            return -1;
        }

        if (convert->add_width_height)
        {
            if (image->width > 255)
            {
                LOG_ERROR("Image \'%s\' width is %d. Maximum width is 255 when using the option \'width-and-height\'.\n",
                    image->path,
                    image->width);
                return -1;
            }

            if (image->height > 255)
            {
                LOG_ERROR("Image \'%s\' height is %d. Maximum height is 255 when using the option \'width-and-height\'.\n",
                    image->path,
                    image->height);
                return -1;
            }
        }

        if (convert_image(convert, image))
        {
            return -1;
        }
    }

    if (convert->tileset_group != NULL)
    {
        struct tileset_group *tileset_group = convert->tileset_group;
        uint32_t j;

        LOG_INFO("Converting tilesets for \'%s\'\n", convert->name);

        for (j = 0; j < tileset_group->nr_tilesets; ++j)
        {
            struct tileset *tileset = &tileset_group->tilesets[j];
            struct image *image = &tileset->image;

            /* assign values to image based on convert flags */
            image->quantize_speed = convert->quantize_speed;
            image->dither = convert->dither;
            image->rotate = convert->rotate;
            image->flip_x = convert->flip_x;
            image->flip_y = convert->flip_y;
            image->transparent_index = convert->transparent_index;
            image->rlet = convert->style == CONVERT_STYLE_RLET;

            image->gfx = false;
            if ((image->rlet || convert->add_width_height) && convert->bpp == BPP_8)
            {
                image->gfx = true;
            }

            LOG_INFO(" - Reading tileset \'%s\'\n",
                image->path);

            if (image_load(image))
            {
                return -1;
            }

            if (convert_tileset(convert, tileset))
            {
                return -1;
            }
        }
    }

    return 0;
}
