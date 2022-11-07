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

#include "image.h"
#include "palette.h"
#include "strings.h"
#include "memory.h"
#include "log.h"

#include "deps/libimagequant/libimagequant.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"

static void swap_pixel(uint32_t *a, uint32_t *b)
{
    uint32_t temp = *a;
    *a = *b;
    *b = temp;
}

static void image_flip_y(uint32_t *data, uint32_t width, uint32_t height)
{
    for (uint32_t r = 0; r < height; ++r)
    {
        uint32_t *row = data + (r * width);

        for (uint32_t c = 0; c < width / 2; ++c)
        {
            swap_pixel(row + c,
                       row + width - c - 1);
        }
    }
}

static void image_flip_x(uint32_t *data, uint32_t width, uint32_t height)
{
    for (uint32_t c = 0; c < width; ++c)
    {
        for (uint32_t r = 0; r < height / 2; ++r)
        {
            swap_pixel(data + (r * width) + c,
                       data + (height - 1 - r) * width + c);
        }
    }
}

static int image_rotate_90(uint32_t *data, uint32_t width, uint32_t height)
{
    uint32_t *new_data;
    uint32_t data_size;

    data_size = width * height * 4;
    new_data = memory_alloc(data_size);
    if (new_data == NULL)
    {
        return -1;
    }

    for(uint32_t i = 0; i < height; ++i)
    {
        uint32_t o = (height - 1 - i) * width;

        for(uint32_t j = 0; j < width; ++j)
        {
            new_data[i + j * height] = data[o + j];
        }
    }

    memcpy(data, new_data, data_size);
    free(new_data);

    return 0;
}

void image_init(struct image *image, const char *path)
{
    /* normal intiialization */
    image->path = strings_dup(path);
    image->name = strings_basename(path);
    image->data = NULL;
    image->data_size = 0;
    image->width = 0;
    image->height = 0;

    /* set by convert */
    image->quantize_speed = 1;
    image->dither =  0.0;
    image->rlet = false;
    image->rotate = 0;
    image->flip_x = false;
    image->flip_y = false;
    image->compressed = false;
    image->uncompressed_size = 0;
    image->transparent_index = 0;
}

int image_load(struct image *image)
{
    uint32_t *data;
    uint32_t width;
    uint32_t height;
    int w;
    int h;
    int c;

    data = (uint32_t *)stbi_load(image->path,
                                 &w, &h, &c,
                                 STBI_rgb_alpha);
    if (data == NULL)
    {
        LOG_ERROR("Could not load image \'%s\'.\n", image->path);
        goto error;
    }

    if (w <= 0 || h <= 0 || w > STBI_MAX_DIMENSIONS || h > STBI_MAX_DIMENSIONS)
    {
        LOG_ERROR("Image \'%s\' is too large.\n", image->path);
        goto error;
    }

    /* library output is int, convert to unsigned */
    width = w;
    height = h;

    /* converted nothing, so no data size yet */
    image->data_size = 0;

    if (image->flip_x)
    {
        image_flip_x(data, width, height);
    }

    if (image->flip_y)
    {
        image_flip_y(data, width, height);
    }

    switch (image->rotate)
    {
        default:
            LOG_ERROR("Invalid image rotation \'%u\'.\n",
                image->rotate);
            goto error;

        case 0:
            image->width = width;
            image->height = height;
            image->data = (uint8_t *)data;
            break;

        case 90:
            image->width = height;
            image->height = width;
            if (image_rotate_90(data, width, height))
            {
                goto error;
            }
            image->data = (uint8_t *)data;
            break;

        case 180:
            image->width = width;
            image->height = height;
            image_flip_y(data, width, height);
            image_flip_x(data, width, height);
            image->data = (uint8_t *)data;
            break;

        case 270:
            image->width = height;
            image->height = width;
            if (image_rotate_90(data, width, height))
            {
                goto error;
            }
            image_flip_y(data, width, height);
            image_flip_x(data, width, height);
            image->data = (uint8_t *)data;
            break;
    }

    return 0;

error:
    free(data);
    return -1;
}

void image_free(struct image *image)
{
    if (image == NULL)
    {
        return;
    }

    free(image->name);
    free(image->path);
    free(image->data);
}

int image_add_width_and_height(struct image *image)
{
    image->data = memory_realloc(image->data, image->data_size + WIDTH_HEIGHT_SIZE);
    if (image->data == NULL)
    {
        return -1;
    }

    memmove(image->data + WIDTH_HEIGHT_SIZE, image->data, image->data_size);

    image->data[0] = image->width;
    image->data[1] = image->height;

    image->data_size += WIDTH_HEIGHT_SIZE;

    return 0;
}

int image_rlet(struct image *image, uint8_t transparent_index)
{
    uint8_t *new_data;
    uint32_t new_size;

    /* multiply by 3 for worst-case encoding */
    new_size = image->width * image->height * 3;
    new_data = memory_alloc(new_size);
    if (new_data == NULL)
    {
        return -1;
    }

    new_size = 0;

    for (uint32_t i = 0; i < image->height; i++)
    {
        uint32_t offset = i * image->width;
        uint32_t left = image->width;

        while (left)
        {
            uint32_t o;
            uint32_t t;

            t = o = 0;
            while (t < left && transparent_index == image->data[t + offset])
            {
                t++;
            }

            new_data[new_size] = t;
            new_size++;

            if ((left -= t))
            {
                uint8_t *opaque_len = &new_data[new_size];
                new_size++;

                while (o < left && transparent_index != image->data[t + o + offset])
                {
                    new_data[new_size] = image->data[t + o + offset];
                    new_size++;
                    o++;
                }

                *opaque_len = o;
                left -= o;
            }

            offset += o + t;
        }
    }

    free(image->data);
    image->data = new_data;
    image->data_size = new_size;

    return 0;
}

int image_set_bpp(struct image *image, bpp_t bpp, uint32_t nr_palette_entries)
{
    uint8_t *new_data;
    uint32_t new_size;
    uint8_t shift_mult;
    uint8_t inc;

    switch (bpp)
    {
        case BPP_1:
            if (nr_palette_entries > 2)
            {
                LOG_ERROR("Palette has too many entries for BPP mode. (max 2)\n");
                return -1;
            }
            shift_mult = 1;
            inc = 8;
            break;
            
        case BPP_2:
            if (nr_palette_entries > 4)
            {
                LOG_ERROR("Palette has too many entries for BPP mode. (max 4)\n");
                return -1;
            }
            shift_mult = 2;
            inc = 4;
            break;

        case BPP_4:
            if (nr_palette_entries > 16)
            {
                LOG_ERROR("Palette has too many entries for BPP mode. (max 16)\n");
                return -1;
            }
            shift_mult = 4;
            inc = 2;
            break;

        case BPP_8:
            return 0;

        default:
            LOG_ERROR("Invalid BPP mode.\n");
            return -1;
    }

    /* if not a multiple of the bit width, reject the image */
    if (image->width % inc)
    {
        LOG_ERROR("Image width is not a multiple of the BPP (needs to be multiple of %d).\n", inc);
        return -1;
    }

    new_data = memory_realloc_array(NULL, image->width, image->height);
    if (new_data == NULL)
    {
        return -1;
    }

    new_size = 0;

    for (uint32_t j = 0; j < image->height; ++j)
    {
        uint32_t offset = j * image->width;

        for (uint32_t k = 0; k < image->width; k += inc)
        {
            uint8_t cur_inc = inc;
            uint8_t col;
            uint8_t byte = 0;

            for (col = 0; col < inc; col++)
            {
                byte |= image->data[k + offset + col] << (--cur_inc * shift_mult);
            }

            new_data[new_size] = byte;
            new_size++;
        }
    }

    free(image->data);
    image->data = new_data;
    image->data_size = new_size;

    return 0;
}

int image_add_offset(struct image *image, uint8_t offset)
{
    for (uint32_t i = 0; i < image->data_size; ++i)
    {
        image->data[i] += offset;
    }

    return 0;
}

int image_remove_omits(struct image *image, const uint8_t *omit_indices, uint32_t nr_omit_indices)
{
    uint8_t *new_data;
    uint32_t new_size;

    if (nr_omit_indices == 0)
    {
        return 0;
    }

    new_size = 0;
    new_data = memory_alloc(image->data_size);
    if (new_data == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < image->data_size; ++i)
    {        
        for (uint32_t j = 0; j < nr_omit_indices; ++j)
        {
            if (image->data[i] == omit_indices[j])
            {
                goto nextbyte;
            }
        }

        new_data[new_size] = image->data[i];
        new_size++;

nextbyte:
        continue;
    }

    free(image->data);
    image->data = new_data;
    image->data_size = new_size;

    return 0;
}

int image_compress(struct image *image, compress_mode_t mode)
{
    size_t size = image->data_size;

    if (compress_array(image->data, &size, mode))
    {
        return -1;
    }

    image->data_size = size;

    return 0;
}

int image_quantize(struct image *image, const struct palette *palette)
{
    liq_image *liqimage = NULL;
    liq_result *liqresult = NULL;
    liq_attr *liqattr = NULL;
    uint8_t *new_data = NULL;
    uint32_t new_size;
    bool bad_alpha;

    liqattr = liq_attr_create();
    if (liqattr == NULL)
    {
        LOG_ERROR("Failed to create image attributes \'%s\'\n", image->path);
        return -1;
    }

    liq_set_speed(liqattr, image->quantize_speed);
    liq_set_max_colors(liqattr, palette->nr_entries);
    liqimage = liq_image_create_rgba(liqattr,
                                     image->data,
                                     image->width,
                                     image->height,
                                     0);
    if (liqimage == NULL)
    {
        LOG_ERROR("Failed to create image \'%s\'\n", image->path);
        liq_attr_destroy(liqattr);
        return -1;
    }

    for (uint32_t i = 0; i < palette->nr_entries; ++i)
    {
        const struct color *c = &palette->entries[i].color;
        liq_color color =
        {
            .r = c->r,
            .g = c->g,
            .b = c->b,
            .a = 255,
        };

        liq_image_add_fixed_color(liqimage, color);
    }

    liqresult = liq_quantize_image(liqattr, liqimage);
    if (liqresult == NULL)
    {
        LOG_ERROR("Failed to quantize image \'%s\'\n",
            image->path);
        liq_image_destroy(liqimage);
        liq_attr_destroy(liqattr);
        return -1;
    }

    liq_set_dithering_level(liqresult, image->dither);

    new_size = image->width * image->height;
    new_data = memory_alloc(new_size);
    if (new_data == NULL)
    {
        liq_result_destroy(liqresult);
        liq_image_destroy(liqimage);
        liq_attr_destroy(liqattr);
        return -1;
    }

    liq_write_remapped_image(liqresult, liqimage, new_data, new_size);

    bad_alpha = false;

    /* loop through each input pixel and insert exact fixed colors */
    for (uint32_t i = 0; i < image->width * image->height; ++i)
    {
        uint32_t offset = i * 4;
        uint8_t r = image->data[offset + 0];
        uint8_t g = image->data[offset + 1];
        uint8_t b = image->data[offset + 2];
        uint8_t a = image->data[offset + 3];

        /* if alpha == 0, this is a transparent pixel */
        if (a == 0)
        {
            new_data[i] = image->transparent_index;
            continue;
        }

        /* otherwise, the user might get bad colors */
        if (a != 255)
        {
            bad_alpha = true;
        }

        for (uint32_t j = 0; j < palette->nr_fixed_entries; ++j)
        {
            const struct palette_entry *fixed = &palette->fixed_entries[j];

            if (!fixed->exact)
            {
                continue;
            }

            if (r == fixed->orig_color.r &&
                g == fixed->orig_color.g &&
                b == fixed->orig_color.b)
            {
                new_data[i] = fixed->index;
            }
        }
    }

    liq_result_destroy(liqresult);
    liq_image_destroy(liqimage);
    liq_attr_destroy(liqattr);

    free(image->data);
    image->data = new_data;
    image->data_size = new_size;

    if (bad_alpha)
    {
        LOG_WARNING("Image has pixels with an alpha not 0 or 255.\n");
        LOG_WARNING("This may result in incorrect color conversion.\n");
    }

    return 0;
}

int image_direct_convert(struct image *image, color_format_t fmt)
{
    bool bad_alpha;
    uint8_t *new_data;
    uint8_t *dst;
    uint32_t new_size;

    switch (fmt)
    {
        case COLOR_1555_GRGB:
        case COLOR_565_RGB:
        case COLOR_565_BGR:
            new_size = image->width * image->height * 2;
            break;

        case COLOR_888_RGB:
        case COLOR_888_BGR:
            new_size = image->width * image->height * 3;
            break;

        default:
            return -1;
    }

    new_data = memory_alloc(new_size);
    if (new_data == NULL)
    {
        return -1;
    }

    dst = new_data;
    bad_alpha = false;

    /* loop through each input pixel and output new format */
    for (uint32_t i = 0; i < image->width * image->height; ++i)
    {
        uint32_t offset = i * 4;
        struct color color;
        uint16_t target;
        uint8_t alpha;

        color.r = image->data[offset + 0];
        color.g = image->data[offset + 1];
        color.b = image->data[offset + 2];
        alpha = image->data[offset + 3];

        /* the user might get bad colors if alpha is set */
        if (alpha != 255)
        {
            bad_alpha = true;
        }

        /* convert the color and store to the new data array */
        switch (fmt)
        {
            case COLOR_1555_GRGB:
                target = color_to_1555_grgb(&color);
                *dst++ = target & 255;
                *dst++ = (target >> 8) & 255;
                break;

            case COLOR_565_RGB:
                target = color_to_565_rgb(&color);
                *dst++ = target & 255;
                *dst++ = (target >> 8) & 255;
                break;
            
            case COLOR_565_BGR:
                target = color_to_565_bgr(&color);
                *dst++ = target & 255;
                *dst++ = (target >> 8) & 255;
                break;

            case COLOR_888_BGR:
                *dst++ = color.r;
                *dst++ = color.g;
                *dst++ = color.b;
                break;

            case COLOR_888_RGB:
                *dst++ = color.b;
                *dst++ = color.g;
                *dst++ = color.r;
                break;

            default:
                break;
        }
    }

    free(image->data);
    image->data = new_data;
    image->data_size = new_size;

    if (bad_alpha)
    {
        LOG_WARNING("Image has pixels with a transparent alpha channel.\n");
        LOG_WARNING("This may result in incorrect color conversion.\n");
    }

    return 0;
}
