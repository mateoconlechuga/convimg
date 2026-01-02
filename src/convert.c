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

#include "convert.h"
#include "strings.h"
#include "compress.h"
#include "memory.h"
#include "tileset.h"
#include "log.h"
#include "image.h"
#include "thread.h"

#include <string.h>
#include <glob.h>

struct conv
{
    struct convert *convert;
    struct image *image;
};

struct convert *convert_alloc(void)
{
    struct convert *convert = memory_alloc(sizeof(struct convert));
    if (convert == NULL)
    {
        return NULL;
    }

    convert->images = NULL;
    convert->nr_images = 0;
    convert->compress = COMPRESS_NONE;
    convert->palette = NULL;
    convert->palette_offset = 0;
    convert->style = CONVERT_STYLE_PALETTE;
    convert->nr_omit_indices = 0;
    convert->width_height = CONVERT_ADD_WIDTH_HEIGHT;
    convert->transparent_index = 0;
    convert->bpp = BPP_8;
    convert->color_fmt = COLOR_565_RGB;
    convert->name = NULL;
    convert->prefix_string = NULL;
    convert->suffix_string = NULL;
    convert->palette_name = NULL;
    convert->quantize_speed = CONVERT_DEFAULT_QUANTIZE_SPEED;
    convert->dither = 0;
    convert->rotate = 0;
    convert->flip_x = false;
    convert->flip_y = false;
    convert->tilesets = NULL;
    convert->nr_tilesets = 0;
    convert->tile_height = 0;
    convert->tile_width = 0;
    convert->tile_rotate = 0;
    convert->tile_flip_x = false;
    convert->tile_flip_y = false;
    convert->p_table = true;

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

    convert->images = memory_realloc_array(convert->images, convert->nr_images + 1, sizeof(struct image));
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

static int convert_add_tileset(struct convert *convert, const char *path)
{
    struct tileset *tileset;
    struct image *image;

    if (convert == NULL || path == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    convert->tilesets = memory_realloc_array(convert->tilesets, convert->nr_tilesets + 1, sizeof(struct tileset));
    if (convert->tilesets == NULL)
    {
        return -1;
    }

    tileset = &convert->tilesets[convert->nr_tilesets];
    convert->nr_tilesets++;

    tileset->tiles = NULL;
    tileset->nr_tiles = 0;

    image = &tileset->image;
    image->path = strings_dup(path);
    image->name = strings_basename(path);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->compressed = false;
    image->rlet = false;
    image->rotate = 0;
    image->flip_x = false;
    image->flip_y = false;
    image->swap_width_height = false;

    return 0;
}

int convert_add_image_path(struct convert *convert, const char *path)
{
    static glob_t globbuf;
    char **paths = NULL;
    char *realPath;
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

    for (int i = 0; i < len; ++i)
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
    char *real_path;
    int len;

    if (convert == NULL || path == NULL)
    {
        return -1;
    }

    real_path = strings_find_images(path, &globbuf);
    if (real_path == NULL)
    {
        return -1;
    }

    paths = globbuf.gl_pathv;
    len = globbuf.gl_pathc;

    if (len == 0)
    {
        LOG_ERROR("Could not find file(s): \'%s\'\n", real_path);
        globfree(&globbuf);
        free(real_path);
        return -1;
    }

    for (int i = 0; i < len; ++i)
    {
        convert_add_tileset(convert, paths[i]);
    }

    globfree(&globbuf);
    free(real_path);

    return 0;
}

void convert_free(struct convert *convert)
{
    if (convert == NULL)
    {
        return;
    }

    for (uint32_t i = 0; i < convert->nr_images; ++i)
    {
        image_free(&convert->images[i]);
    }
    convert->nr_images = 0;

    for (uint32_t i = 0; i < convert->nr_tilesets; ++i)
    {
        tileset_free_tiles(&convert->tilesets[i]);
    }
    free(convert->tilesets);
    convert->nr_tilesets = 0;

    free(convert->images);
    convert->images = NULL;

    free(convert->name);
    convert->name = NULL;

    free(convert->palette_name);
    convert->palette_name = NULL;

    free(convert->prefix_string);
    convert->prefix_string = NULL;

    free(convert->suffix_string);
    convert->suffix_string = NULL;
}

static int convert_find_palette(struct convert *convert, struct palette **palettes, uint32_t nr_palettes)
{
    if (palettes == NULL || convert->palette_name == NULL)
    {
        goto error;
    }

    for (uint32_t i = 0; i < nr_palettes; ++i)
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

static bool convert_image(struct convert *convert, struct image *image)
{
    if (convert_is_palette_style(convert))
    {
        if (image_quantize(image, convert->palette))
        {
            return false;
        }

        if (convert->palette_offset != 0)
        {
            if (convert->palette_offset + convert->palette->nr_entries >=
                PALETTE_MAX_ENTRIES)
            {
                LOG_ERROR("Palette offset places indices out of range for "
                        "convert \'%s\'\n",
                    convert->name);
                return false;
            }

            if (image_add_offset(image, convert->palette_offset))
            {
                return false;
            }
        }

        if (convert->style == CONVERT_STYLE_RLET)
        {
            if (image_rlet(image, convert->palette_offset + convert->transparent_index))
            {
                return false;
            }
        }

        if (convert->nr_omit_indices)
        {
            if (image_remove_omits(image, convert->omit_indices, convert->nr_omit_indices))
            {
                return false;
            }
        }

        if (convert->bpp != BPP_8)
        {
            if (image_set_bpp(image, convert->bpp, convert->palette->nr_entries))
            {
                return false;
            }
        }
    }
    else
    {
        if (image_direct_convert(image, convert->color_fmt))
        {
            return false;
        }
    }

    if (convert->width_height == CONVERT_ADD_WIDTH_HEIGHT ||
        convert->width_height == CONVERT_SWAP_WIDTH_HEIGHT)
    {
        if (image_add_width_and_height(image, convert->width_height == CONVERT_SWAP_WIDTH_HEIGHT))
        {
            return false;
        }
    }

    image->uncompressed_size = image->data_size;

    if (convert->compress != COMPRESS_NONE)
    {
        if (image_compress(image, convert->compress))
        {
            return false;
        }

        image->compressed = true;
    }

    return true;
}

static bool convert_image_thread(void *arg)
{
    struct conv *conv = arg;
    struct convert *convert = conv->convert;
    struct image *image = conv->image;

    /* assign image constants from convert */
    image->quantize_speed = convert->quantize_speed;
    image->dither = convert->dither;
    image->rotate = convert->rotate;
    image->flip_x = convert->flip_x;
    image->flip_y = convert->flip_y;
    image->swap_width_height = convert->width_height == CONVERT_SWAP_WIDTH_HEIGHT;
    image->transparent_index = convert->transparent_index;
    image->rlet = convert->style == CONVERT_STYLE_RLET;

    if (convert->prefix_string)
    {
        char *tmp = strings_concat(convert->prefix_string, image->name, 0);
        free(image->name);
        image->name = tmp;
    }
    if (convert->suffix_string)
    {
        char *tmp = strings_concat(image->name, convert->suffix_string, 0);
        free(image->name);
        image->name = tmp;
    }

    image->gfx = false;
    if ((image->rlet || convert->width_height != CONVERT_NO_WIDTH_HEIGHT) && convert->bpp == BPP_8)
    {
        image->gfx = true;
    }

    LOG_INFO(" - Reading image \'%s\'\n", image->path);

    if (image_load(image))
    {
        return -1;
    }

    if (convert->width_height != CONVERT_NO_WIDTH_HEIGHT)
    {
        if (image->width > 255)
        {
            LOG_ERROR("Image \'%s\' width is %u. "
                "Maximum width is 255 when using the option \'width-and-height\'.\n",
                image->name,
                image->width);
            return false;
        }

        if (image->height > 255)
        {
            LOG_ERROR("Image \'%s\' height is %u. "
                "Maximum height is 255 when using the option \'width-and-height\'.\n",
                image->name,
                image->height);
            return false;
        }
    }

    bool ret = convert_image(convert, image);

    free(arg);

    return ret;
}

static bool convert_tileset(struct convert *convert, struct tileset *tileset)
{
    uint32_t nr_tiles;
    uint32_t x;
    uint32_t y;

    if (!tileset->tile_width || tileset->image.width % tileset->tile_width)
    {
        LOG_ERROR("Image dimensions do not support tile width.\n");
        return false;
    }

    if (!tileset->tile_height || tileset->image.height % tileset->tile_height)
    {
        LOG_ERROR("Image dimensions do not support tile height.\n");
        return false;
    }

    nr_tiles =
        (tileset->image.width / tileset->tile_width) *
        (tileset->image.height / tileset->tile_height);

    if (tileset_alloc_tiles(tileset, nr_tiles))
    {
        return false;
    }

    tileset->rlet = convert->style == CONVERT_STYLE_RLET;
    tileset->compressed = convert->compress != COMPRESS_NONE;

    y = 0;
    x = 0;

    for (uint32_t i = 0; i < tileset->nr_tiles; ++i)
    {
        uint32_t tile_dim = tileset->tile_width * tileset->tile_height;
        uint32_t tile_data_size = tile_dim * sizeof(uint32_t);
        uint32_t tile_stride = tileset->tile_width * sizeof(uint32_t);
        uint32_t image_stride = tileset->image.width * sizeof(uint32_t);
        void *tile_data = tileset->tiles[i].data;
        uint8_t *dst;

        struct image tile =
        {
            .data = tile_data,
            .data_size = tile_data_size,
            .width = tileset->tile_width,
            .height = tileset->tile_height,
            .transparent_index = convert->transparent_index,
            .quantize_speed = convert->quantize_speed,
            .dither = convert->dither,
            .name = NULL,
            .path = NULL,
        };

        dst = tile_data;

        for (uint32_t j = 0; j < tile.height; ++j)
        {
            uint32_t o = (j * image_stride) + y;

            memcpy(dst, &tileset->image.data[x + o], tile_stride);

            dst += tile_stride;
        }

        if (tileset->tile_flip_x)
        {
            image_flip_x(tile_data, tile.width, tile.height);
        }

        if (tileset->tile_flip_y)
        {
            image_flip_y(tile_data, tile.width, tile.height);
        }

        switch (tileset->tile_rotate)
        {
            default:
            case 0:
                break;

            case 90:
                tile.width = tileset->tile_height;
                tile.height = tileset->tile_width;
                if (image_rotate_90(tile_data, tile.width, tile.height))
                {
                    goto error;
                }
                break;

            case 180:
                image_flip_y(tile_data, tile.width, tile.height);
                image_flip_x(tile_data, tile.width, tile.height);
                break;

            case 270:
                tile.width = tileset->tile_height;
                tile.height = tileset->tile_width;
                if (image_rotate_90(tile_data, tile.width, tile.height))
                {
                    goto error;
                }
                image_flip_y(tile_data, tile.width, tile.height);
                image_flip_x(tile_data, tile.width, tile.height);
                break;
        }

        x += tile_stride;

        if (x >= image_stride)
        {
            x = 0;
            y += tileset->tile_height * image_stride;
        }

        if (!convert_image(convert, &tile))
        {
error:
            free(tile.data);
            return false;
        }

        tileset->tiles[i].data_size = tile.data_size;
        tileset->tiles[i].data = tile.data;
    }

    return true;
}

int convert_generate(struct convert *convert, struct palette **palettes, uint32_t nr_palettes)
{
    if (convert->nr_images == 0 && convert->nr_tilesets == 0)
    {
        LOG_WARNING("No images or tilesets in convert \'%s\'\n",
            convert->name);
        return 0;
    }

    LOG_INFO("Generating convert \'%s\'\n", convert->name);

    if (convert_is_palette_style(convert))
    {
        if (convert_find_palette(convert, palettes, nr_palettes))
        {
            return -1;
        }
    }

    for (uint32_t i = 0; i < convert->nr_images; ++i)
    {
        struct conv *conv = malloc(sizeof(struct conv));
        if (!conv)
        {
            return -1;
        }

        conv->convert = convert;
        conv->image = &convert->images[i];

        if (!thread_start(convert_image_thread, conv))
        {
            return -1;
        }
    }

    for (uint32_t j = 0; j < convert->nr_tilesets; ++j)
    {
        struct tileset *tileset = &convert->tilesets[j];
        struct image *image = &tileset->image;

        /* assign tileset constants from convert */
        tileset->tile_height = convert->tile_height;
        tileset->tile_width = convert->tile_width;
        tileset->tile_rotate = convert->tile_rotate;
        tileset->tile_flip_x = convert->tile_flip_x;
        tileset->tile_flip_y = convert->tile_flip_y;
        tileset->p_table = convert->p_table;

        /* assign image constants from convert */
        image->quantize_speed = convert->quantize_speed;
        image->dither = convert->dither;
        image->rotate = convert->rotate;
        image->flip_x = convert->flip_x;
        image->flip_y = convert->flip_y;
        image->swap_width_height = convert->width_height == CONVERT_SWAP_WIDTH_HEIGHT;
        image->transparent_index = convert->transparent_index;
        image->rlet = convert->style == CONVERT_STYLE_RLET;

        if (convert->prefix_string)
        {
            char *tmp = strings_concat(convert->prefix_string, image->name, 0);
            free(image->name);
            image->name = tmp;
        }
        if (convert->suffix_string)
        {
            char *tmp = strings_concat(image->name, convert->suffix_string, 0);
            free(image->name);
            image->name = tmp;
        }

        image->gfx = false;
        if ((image->rlet || convert->width_height != CONVERT_NO_WIDTH_HEIGHT) && convert->bpp == BPP_8)
        {
            image->gfx = true;
        }

        LOG_INFO(" - Reading tileset \'%s\'\n", image->path);

        if (image_load(image))
        {
            return -1;
        }

        if (!convert_tileset(convert, tileset))
        {
            return -1;
        }
    }

    return 0;
}
