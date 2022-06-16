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
#include "log.h"

#include "deps/libimagequant/libimagequant.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"

#include <math.h>

static void swap_pixel(uint32_t *a, uint32_t *b)
{
    uint32_t temp = *a;
    *a = *b;
    *b = temp;
}

static void image_flip_y(uint32_t *image, int width, int height)
{
    int r;

    for (r = 0; r < height; ++r)
    {
        uint32_t *row = image + (r * width);
        int c;

        for (c = 0; c < width / 2; ++c)
        {
            swap_pixel(row + c,
                       row + width - c - 1);
        }
    }
}

static void image_flip_x(uint32_t *image, int width, int height)
{
    int c;

    for (c = 0; c < width; ++c)
    {
        int r;

        for (r = 0; r < height / 2; ++r)
        {
            swap_pixel(image + (r * width) + c,
                       image + (height - 1 - r) * width + c);
        }
    }
}

static void image_rotate_90(uint32_t *image, int width, int height)
{
    const size_t size = (size_t)width * (size_t)height;
    uint32_t *newimage = malloc(size * sizeof(uint32_t));
    int i, j;

    for(i = 0; i < height; ++i)
    {
        int o = (height - 1 - i) * width;
        for(j = 0; j < width; ++j)
        {
            newimage[i + j * height] = image[o + j];
        }
    }

    memcpy(image, newimage, size * sizeof(uint32_t));
    free(newimage);
}

int image_load(struct image *image)
{
    int width;
    int height;
    int channels;
    uint32_t *data;

    data = (uint32_t*)stbi_load(image->path,
                                &width,
                                &height,
                                &channels,
                                STBI_rgb_alpha);
    if (data == NULL)
    {
        LOG_ERROR("Could not load image \'%s\'\n", image->path);
        return -1;
    }

    image->orig_size = image->size = width * height;
    image->compressed = false;
    image->bad_alpha = false;

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
            LOG_WARNING("Invalid image rotation \'%d\' -- using 0 degrees.\n",
                image->rotate);
            // fall through
        case 0:
            image->width = width;
            image->height = height;
            image->data = (uint8_t*)data;
            break;

        case 90:
            image->width = height;
            image->height = width;
            image_rotate_90(data, width, height);
            image->data = (uint8_t*)data;
            break;

        case 180:
            image->width = width;
            image->height = height;
            image_flip_y(data, width, height);
            image_flip_x(data, width, height);
            image->data = (uint8_t*)data;
            break;

        case 270:
            image->width = height;
            image->height = width;
            image_rotate_90(data, width, height);
            image_flip_y(data, width, height);
            image_flip_x(data, width, height);
            image->data = (uint8_t*)data;
            break;
    }

    return 0;
}

void image_free(struct image *image)
{
    if( image == NULL )
    {
        return;
    }

    free(image->name);
    free(image->path);
    free(image->data);
}

int image_add_width_and_height(struct image *image)
{
    if (image == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    image->data = realloc(image->data, image->size + WIDTH_HEIGHT_SIZE);
    if (image->data == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    memmove(image->data + WIDTH_HEIGHT_SIZE, image->data, image->size);

    image->data[0] = image->width;
    image->data[1] = image->height;

    image->size += WIDTH_HEIGHT_SIZE;

    return 0;
}

int image_rlet(struct image *image, int transparent_index)
{
    uint8_t *new_data;
    size_t size;
    int new_size = 0;
    int i;

    if (transparent_index < 0)
    {
        LOG_ERROR("Transparent color index not specified for RLET mode.\n");
        return -1;
    }

    size = (size_t)image->width * (size_t)image->height * (size_t)2;
    new_data = malloc(size);
    if (new_data == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    for (i = 0; i < image->height; i++)
    {
        int offset = i * image->width;
        int left = image->width;

        while (left)
        {
            int o, t;

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
    image->size = new_size;

    return 0;
}

int image_set_bpp(struct image *image, bpp_t bpp, int nr_palette_entries)
{
    int shift;
    int shift_mult;
    int inc;
    int j, k;
    int new_size;
    uint8_t *new_data;

    switch (bpp)
    {
        case BPP_1:
            if (nr_palette_entries > 2)
            {
                LOG_ERROR("Palette has too many entries for BPP mode. (max 2)\n");
                return -1;
            }
            shift = 3;
            shift_mult = 1;
            break;
        case BPP_2:
            if (nr_palette_entries > 4)
            {
                LOG_ERROR("Palette has too many entries for BPP mode. (max 4)\n");
                return -1;
            }
            shift = 2;
            shift_mult = 2;
            break;
        case BPP_4:
            if (nr_palette_entries > 16)
            {
                LOG_ERROR("Palette has too many entries for BPP mode. (max 16)\n");
                return -1;
            }
            shift = 1;
            shift_mult = 4;
            break;
        case BPP_8:
            return 0;
        default:
            LOG_ERROR("Invalid BPP mode.\n");
            return -1;
    }

    new_size = 0;
    new_data = malloc(image->size);
    if (new_data == NULL)
    {
        LOG_ERROR("Memory error in \'%s\'.\n", __func__);
        return -1;
    }

    inc = pow(2, shift);

    for (j = 0; j < image->height; ++j)
    {
        int line = j * image->width;

        for (k = 0; k < image->width; k += inc)
        {
            int cur_inc = inc;
            int col;
            uint8_t byte = 0;

            for (col = 0; col < inc; col++)
            {
                byte |= image->data[k + line + col] << (--cur_inc * shift_mult);
            }

            new_data[new_size] = byte;
            new_size++;
        }
    }

    free(image->data);
    image->data = new_data;

    image->size = (int)ceilf((float)image->width / (float)inc) * image->height;

    return 0;
}

int image_add_offset(struct image *image, int offset)
{
    int i;

    for (i = 0; i < image->height * image->width; ++i)
    {
        image->data[i] += offset;
    }

    return 0;
}

int image_remove_omits(struct image *image, int *omit_indices, int nr_omit_indices)
{
    int i, j;
    int new_size = 0;
    uint8_t *new_data;

    if (nr_omit_indices == 0)
    {
        return 0;
    }

    new_data = malloc(image->size);
    if (new_data == NULL)
    {
        return -1;
    }

    for (i = 0; i < image->size; ++i)
    {
        for (j = 0; j < nr_omit_indices; ++j)
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
    image->size = new_size;

    return 0;
}

int image_compress(struct image *image, compress_t compress)
{
    size_t new_size;
    int ret = 0;

    if (image == NULL)
    {
        LOG_ERROR("Invalid param in \'%s\'. Please contact the developer.\n", __func__);
        return -1;
    }

    if (compress == COMPRESS_NONE)
    {
        return 0;
    }

    new_size = image->size;
    ret = compress_array(image->data, &new_size, compress);
    image->size = new_size;

    return ret;
}

int image_quantize(struct image *image, struct palette *palette)
{
    liq_image *liqimage = NULL;
    liq_result *liqresult = NULL;
    liq_attr *liqattr = NULL;
    uint8_t *data = NULL;
    int i;

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

    for (i = 0; i < palette->nr_entries; ++i)
    {
        liq_image_add_fixed_color(liqimage, palette->entries[i].color.rgb);
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

    data = malloc(image->size);
    if (data == NULL)
    {
        LOG_ERROR("Failed to allocate \'%s\'\n", image->path);
        liq_result_destroy(liqresult);
        liq_image_destroy(liqimage);
        liq_attr_destroy(liqattr);
        return -1;
    }

    liq_write_remapped_image(liqresult, liqimage, data, image->size);

    // loop through each input pixel and insert exact fixed colors
    for (i = 0; i < image->size; ++i)
    {
        int offset = i * 4;
        uint8_t r, g, b, a;
        int j;

        r = image->data[offset + 0];
        g = image->data[offset + 1];
        b = image->data[offset + 2];
        a = image->data[offset + 3];

        /* if alpha == 0, this is a transparent pixel */
        if (a == 0)
        {
            if (image->transparent_index < 0)
            {
                LOG_ERROR("Encountered pixel alpha of 0, but `transparent-index` is not set.\n");
                return -1;
            }

            data[i] = image->transparent_index;
            continue;
        }

        /* otherwise, the user might get bad colors */
        if (a != 255)
        {
            image->bad_alpha = true;
        }

        for (j = 0; j < palette->nr_fixed_entries; ++j)
        {
            struct palette_entry *fixed_entry = &palette->fixed_entries[j];
            if (!fixed_entry->exact)
            {
                continue;
            }

            if (r == fixed_entry->orig_color.rgb.r &&
                g == fixed_entry->orig_color.rgb.g &&
                b == fixed_entry->orig_color.rgb.b)
            {
                data[i] = fixed_entry->index;
            }
        }
    }

    free(image->data);
    image->data = data;

    liq_result_destroy(liqresult);
    liq_image_destroy(liqimage);
    liq_attr_destroy(liqattr);

    return 0;
}
