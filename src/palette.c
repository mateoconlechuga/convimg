/*
 * Copyright 2017-2024 Matt "MateoConLechuga" Waltz
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

#include "palette.h"
#include "convert.h"
#include "memory.h"
#include "strings.h"
#include "image.h"
#include "log.h"
#include "thread.h"

#include "deps/libimagequant/libimagequant.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <glob.h>

/* maximum number of colors that can be quantized */
#define MAX_NR_COLORS 536870912

/* built-in palettes */
static uint8_t palette_xlibc[];
static uint8_t palette_rgb332[];

struct palette *palette_alloc(void)
{
    struct palette *palette = NULL;
    unsigned int i;

    palette = memory_alloc(sizeof(struct palette));
    if (palette == NULL)
    {
        return NULL;
    }

    palette->images = NULL;
    palette->nr_images = 0;
    palette->max_entries = PALETTE_MAX_ENTRIES;
    palette->nr_entries = 0;
    palette->nr_fixed_entries = 0;
    palette->color_fmt = COLOR_1555_GRGB;
    palette->quantize_speed = PALETTE_DEFAULT_QUANTIZE_SPEED;
    palette->automatic = false;
    palette->name = NULL;

    for (i = 0; i < PALETTE_MAX_ENTRIES; ++i)
    {
        struct palette_entry *entry = &palette->entries[i];
        entry->index = i;
        entry->exact = false;
        entry->valid = false;
        entry->fixed = false;
    }

    return palette;
}

static int palette_add_image(struct palette *palette, const char *path)
{
    struct image *image;

    if (palette == NULL || path == NULL)
    {
        return -1;
    }

    palette->images = memory_realloc_array(palette->images, palette->nr_images + 1, sizeof(struct image));
    if (palette->images == NULL)
    {
        return -1;
    }

    image = &palette->images[palette->nr_images];
    palette->nr_images++;

    image->name = strings_basename(path);
    image->data = NULL;
    image->width = 0;
    image->height = 0;
    image->rotate = 0;
    image->flip_x = false;
    image->flip_y = false;
    image->rlet = false;

    image->path = strings_dup(path);
    if (image->path == NULL)
    {
        return -1;
    }

    LOG_DEBUG("Adding image: %s [%s]\n", image->path, image->name);

    return 0;
}

int pallete_add_path(struct palette *palette, const char *path)
{
    static glob_t globbuf;
    char **paths = NULL;
    char *real_path;
    size_t len;

    if (palette == NULL || path == NULL)
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

    for (size_t i = 0; i < len; ++i)
    {
        palette_add_image(palette, paths[i]);
    }

    globfree(&globbuf);
    free(real_path);

    return 0;
}

void palette_free(struct palette *palette)
{
    if (palette == NULL)
    {
        return;
    }

    for (uint32_t i = 0; i < palette->nr_images; ++i)
    {
        struct image *image = &palette->images[i];

        free(image->name);
        image->name = NULL;

        free(image->path);
        image->path = NULL;

        free(image->data);
        image->path = NULL;
    }

    free(palette->images);
    palette->images = NULL;

    free(palette->name);
    palette->name = NULL;
}

void palette_generate_builtin(struct palette *palette,
                              const uint8_t *builtin,
                              uint32_t nr_entries)
{
    for (uint32_t i = 0; i < nr_entries; ++i)
    {
        struct color *color = &palette->entries[i].color;
        const uint32_t offset = i * 3;

        color->r = builtin[offset + 0];
        color->g = builtin[offset + 1];
        color->b = builtin[offset + 2];

        color_normalize(color, palette->color_fmt);
    }

    palette->nr_entries = nr_entries;
}

/* in automatic mode, read the images from the converts using the palette. */
int palette_automatic_build(struct palette *palette, struct convert **converts, uint32_t nr_converts)
{
    for (uint32_t i = 0; i < nr_converts; ++i)
    {
        const struct convert *convert = converts[i];

        if (strcmp(palette->name, converts[i]->palette_name))
        {
            continue;
        }

        for (uint32_t j = 0; j < converts[i]->nr_images; ++j)
        {
            if (palette_add_image(palette, converts[i]->images[j].path))
            {
                return -1;
            }
        }

        for (uint32_t j = 0; j < convert->nr_tilesets; ++j)
        {
            if (palette_add_image(palette, convert->tilesets[j].image.path))
            {
                return -1;
            }
        }
    }

    return 0;
}

void palette_sort(struct palette *palette)
{
iterate:
    for (uint32_t i = 0; i < palette->nr_entries; ++i)
    {
        struct palette_entry *e1 = &palette->entries[i];

        if (!e1->valid || e1->fixed)
        {
            continue;
        }

        for (uint32_t j = i + 1; j < palette->nr_entries; ++j)
        {
            struct palette_entry *e2 = &palette->entries[j];
            uint8_t r1, r2;
            uint8_t g1, g2;
            uint8_t b1, b2;

            if (!e2->valid || e2->fixed)
            {
                continue;
            }

            r1 = e1->color.r;
            g1 = e1->color.g;
            b1 = e1->color.b;

            r2 = e2->color.r;
            g2 = e2->color.g;
            b2 = e2->color.b;

            if ((r2 < r1) ||
                (r2 == r1 && g2 < g1) ||
                (r2 == r1 && g2 == g1 && b2 < b1))
            {
                struct palette_entry tmp = *e1;
                *e1 = *e2;
                *e2 = tmp;
                goto iterate;
            }
        }
    }
}

static uint32_t palette_nr_exact_fixed_entries(const struct palette *palette)
{
    uint32_t nr_entries = 0;

    for (uint32_t i = 0; i < palette->nr_fixed_entries; ++i)
    {
        if (palette->fixed_entries[i].exact)
        {
            nr_entries++;
        }
    }

    return nr_entries;
}

static bool palette_is_exact_fixed_entry(const struct palette *palette, const struct color *color)
{
    for (uint32_t k = 0; k < palette->nr_fixed_entries; ++k)
    {
        const struct palette_entry *fixed = &palette->fixed_entries[k];

        if (fixed->exact &&
            color->r == fixed->color.r &&
            color->g == fixed->color.g &&
            color->b == fixed->color.b)
        {
            return true;
        }
    }

    return false;
}

int palette_convert_colors(struct palette *palette)
{
    for (uint32_t i = 0; i < palette->nr_entries; ++i)
    {
        struct palette_entry *entry = &palette->entries[i];

        /* convert the entries into the target format */
        if (entry->valid)
        {
            switch (palette->color_fmt)
            {
                case COLOR_1555_GRGB:
                    entry->target = color_to_1555_grgb(&entry->color);
                    break;

                case COLOR_565_BGR:
                    entry->target = color_to_565_bgr(&entry->color);
                    break;

                case COLOR_565_RGB:
                    entry->target = color_to_565_rgb(&entry->color);
                    break;

                default:
                    return -1;
            }
        }
    }

    return 0;
}

int palette_generate_with_images(struct palette *palette)
{
    const liq_palette *liqpalette;
    liq_error liqerr;
    liq_attr *attr;
    liq_histogram *hist;
    uint32_t max_index;
    uint32_t nr_exact_entries;
    uint32_t nr_max_entries;
    uint32_t nr_unused_entries;
    
    attr = liq_attr_create();

    liq_set_speed(attr, palette->quantize_speed);

    max_index = 0;
    hist = NULL;

    /* set the total number of palette entries that will be */
    /* quantized against (exclude exact entries) */
    nr_exact_entries = palette_nr_exact_fixed_entries(palette);
    if (nr_exact_entries > palette->max_entries)
    {
        LOG_ERROR("More exact fixed entries than entries available.\n");
        return -1;
    }
    nr_max_entries = palette->max_entries - nr_exact_entries;

    LOG_DEBUG("Available quantization colors: %u\n", nr_max_entries);

    liq_set_max_colors(attr, nr_max_entries);

    hist = liq_histogram_create(attr);
 
    /* add fixed colors that are not exact to the quantizer */
    for (uint32_t i = 0; i < palette->nr_fixed_entries; ++i)
    {
        struct palette_entry *entry = &palette->fixed_entries[i];

        if (entry->exact)
        {
            continue;
        }

        /* normalize the input color */
        color_normalize(&entry->color, palette->color_fmt);

        const liq_color color = 
        {
            .r = entry->color.r,
            .g = entry->color.g,
            .b = entry->color.b,
            .a = 255,
        };

        liqerr = liq_histogram_add_fixed_color(hist, color, 0);
        if (liqerr != LIQ_OK)
        {
            LOG_ERROR("Failed to add fixed color. Please contact the developer.\n");
            liq_histogram_destroy(hist);
            liq_attr_destroy(attr);
            return -1;
        }
    }

    size_t nr_colors_alloc = 4096;
    uint32_t *colors = memory_realloc_array(NULL, nr_colors_alloc, 4);
    uint32_t nr_colors = 0;

    /* quantize the images into a palette */
    for (uint32_t i = 0; i < palette->nr_images; ++i)
    {
        struct image *image = &palette->images[i];

        LOG_INFO(" - Reading image \'%s\'\n", image->path);

        if (image_load(image))
        {
            liq_histogram_destroy(hist);
            liq_attr_destroy(attr);
            free(colors);
            return -1;
        }

        const uint32_t *image_rgba = (uint32_t*)image->data;

        for (uint32_t j = 0; j < image->width * image->height; ++j)
        {
            struct color color;

            color.rgba = image_rgba[j];

            /* clamp transparent/semi-transparent pixels */
            color.a = color.a < 128 ? 0 : 255;

            /* don't add exact fixed colors to the palette */
            if (palette_is_exact_fixed_entry(palette, &color))
            {
                continue;
            }

            color_normalize(&color, palette->color_fmt);

            if (nr_colors == MAX_NR_COLORS)
            {
                LOG_ERROR("Too many colors to quantize\n");
                free(colors);
                return -1;
            }

            if (nr_colors == nr_colors_alloc)
            {
                /* multiply by 1.5 for best space allocation */
                nr_colors_alloc *= 3;
                nr_colors_alloc /= 2;

                LOG_DEBUG("%u colors, realloc %u\n",
                    nr_colors,
                    nr_colors_alloc);

                colors = memory_realloc_array(colors, nr_colors_alloc, 4);
                if (colors == NULL)
                {
                    return -1;
                }
            }

            colors[nr_colors] = color.rgba;

            nr_colors++;
        }
    }

    LOG_DEBUG("%u colors in palette before quantization\n", nr_colors);

    if (nr_colors > 0)
    {
        liq_result *liqresult = NULL;
        liq_image *liqimage;

        liqimage = liq_image_create_rgba(attr, colors, 1, nr_colors, 0);
        if (liqimage == NULL)
        {
            LOG_ERROR("Failed to create palette - image may be too large\n");
            liq_histogram_destroy(hist);
            liq_attr_destroy(attr);
            free(colors);
            return -1;
        }

        liqerr = liq_histogram_add_image(hist, attr, liqimage);
        if (liqerr != LIQ_OK)
        {
            LOG_ERROR("Failed to create palette histogram.\n");
            liq_histogram_destroy(hist);
            liq_image_destroy(liqimage);
            liq_attr_destroy(attr);
            free(colors);
            return -1;
        }

        liq_image_destroy(liqimage);
        free(colors);

        liqerr = liq_histogram_quantize(hist, attr, &liqresult);
        if (liqerr != LIQ_OK)
        {
            LOG_ERROR("Failed to quantize palette.\n");
            liq_histogram_destroy(hist);
            liq_attr_destroy(attr);
            return -1;
        }

        liqpalette = liq_get_palette(liqresult);

        /* store the quantized palette */
        for (uint32_t i = 0; i < liqpalette->count; ++i)
        {
            struct color color;

            color.r = liqpalette->entries[i].r;
            color.g = liqpalette->entries[i].g;
            color.b = liqpalette->entries[i].b;
            color.a = 255;

            color_normalize(&color, palette->color_fmt);

            palette->entries[i].color = color;
            palette->entries[i].valid = true;
            palette->entries[i].fixed = false;

            if (i > max_index)
            {
                max_index = i;
            }
        }

        LOG_DEBUG("Number of fixed entries: %u\n", palette->nr_fixed_entries);

        /* add non-exact fixed colors in the quantized palette */
        /* and move them to the correct index */
        for (uint32_t i = 0; i < palette->nr_fixed_entries; ++i)
        {
            struct palette_entry *fixed_entry = &palette->fixed_entries[i];

            if (fixed_entry->exact)
            {
                continue;
            }

            for (uint32_t j = 0; j < liqpalette->count; ++j)
            {
                struct palette_entry *entry = &palette->entries[j];

                if (!entry->fixed &&
                    fixed_entry->color.r == entry->color.r &&
                    fixed_entry->color.g == entry->color.g &&
                    fixed_entry->color.b == entry->color.b)
                {
                    struct palette_entry tmp_entry;

                    tmp_entry = palette->entries[j];
                    palette->entries[j] = palette->entries[fixed_entry->index];
                    palette->entries[fixed_entry->index] = tmp_entry;
                    palette->entries[fixed_entry->index].valid = true;
                    palette->entries[fixed_entry->index].fixed = true;

                    if (fixed_entry->index > max_index)
                    {
                        max_index = fixed_entry->index;
                    }

                    break;
                }
            }
        }

        liq_result_destroy(liqresult);
    }

    /* add exact fixed colors to the palette */
    /* they are just place holders (will be removed in the quantized image) */
    for (uint32_t i = 0; i < palette->nr_fixed_entries; ++i)
    {
        struct palette_entry *fixed_entry = &palette->fixed_entries[i];

        if (!fixed_entry->exact)
        {
            continue;
        }

        color_normalize(&fixed_entry->color, palette->color_fmt);

        /* locate another valid place to store an entry */
        /* if an entry already occupies this location */
        if (palette->entries[fixed_entry->index].valid)
        {
            uint32_t j;

            if (nr_max_entries)
            {
                for (j = 0; j < nr_max_entries; ++j)
                {
                    if (!palette->entries[j].valid)
                    {
                        break;
                    }
                }

                if (j == nr_max_entries)
                {
                     LOG_ERROR("Could not find a valid fixed entry location.\n");
                     liq_histogram_destroy(hist);
                     liq_attr_destroy(attr);
                     return -1;
                }
            }

            palette->entries[j] = palette->entries[fixed_entry->index];

            if (j > max_index)
            {
                max_index = j;
            }
        }

        palette->entries[fixed_entry->index] = *fixed_entry;
        palette->entries[fixed_entry->index].valid = true;
        palette->entries[fixed_entry->index].fixed = true;

        if (fixed_entry->index > max_index)
        {
            max_index = fixed_entry->index;
        }
    }

    palette->nr_entries = max_index + 1;
    nr_unused_entries = 0;

    for (uint32_t i = 0; i < palette->nr_entries; ++i)
    {
        if (!palette->entries[i].valid)
        {
            nr_unused_entries++;
        }
    }

    /* sort the palette to prevent different invocations from messing */
    /* everything up (hashmaps!) */
    palette_sort(palette);

    LOG_INFO("Generated palette \'%s\' with %u colors (%u unused)\n",
            palette->name, palette->nr_entries,
            (nr_max_entries - palette->nr_entries) + nr_unused_entries);

    liq_histogram_destroy(hist);
    liq_attr_destroy(attr);

    return 0;
}

bool palette_generate_thread(void *palette)
{
    return
        !palette_generate_with_images(palette) &&
        !palette_convert_colors(palette);
}

int palette_generate(struct palette *palette, struct convert **converts, uint32_t nr_converts)
{
    if (!strcmp(palette->name, "xlibc"))
    {
        palette_generate_builtin(palette,
                                 palette_xlibc,
                                 PALETTE_MAX_ENTRIES);
        return 0;
    }
    else if (!strcmp(palette->name, "rgb332"))
    {
        palette_generate_builtin(palette,
                                 palette_rgb332,
                                 PALETTE_MAX_ENTRIES);
        return 0;
    }

    LOG_INFO("Generating palette \'%s\'\n", palette->name);

    if (palette->automatic)
    {
        if (palette_automatic_build(palette, converts, nr_converts))
        {
            LOG_ERROR("Failed building automatic palette.\n");
            return -1;
        }
    }

    if (palette->nr_fixed_entries > palette->max_entries)
    {
        LOG_ERROR("Number of fixed colors exceeds maximum palette size "
                 "in palette \'%s\'\n", palette->name);
        return -1;
    }

    if (palette->nr_images > 0)
    {
        if (!thread_start(palette_generate_thread, palette))
        {
            return -1;
        }
        return 0;
    }
    else
    {
        uint32_t nr_entries = 0;

        LOG_WARNING("Creating palette \'%s\' without images\n", palette->name);

        if (palette->nr_fixed_entries == 0)
        {
            LOG_ERROR("No fixed colors to create palette \'%s\' with.\n",
                palette->name);
            return -1;
        }

        for (uint32_t i = 0; i < palette->nr_fixed_entries; ++i)
        {
            struct palette_entry *entry = &palette->fixed_entries[i];

            color_normalize(&entry->color, palette->color_fmt);

            palette->entries[entry->index] = *entry;
            palette->entries[entry->index].valid = true;

            if (nr_entries < (entry->index + 1))
            {
                nr_entries = entry->index + 1;
            }
        }

        palette->nr_entries = nr_entries;

        LOG_INFO("Generated palette \'%s\' with %u colors (%u unused)\n",
                palette->name, palette->nr_entries,
                PALETTE_MAX_ENTRIES - palette->nr_entries + palette->nr_fixed_entries);
    }

    return palette_convert_colors(palette);
}

static uint8_t palette_xlibc[] =
{
    0x00,0x00,0x00,
    0x00,0x20,0x08,
    0x00,0x41,0x10,
    0x00,0x61,0x18,
    0x00,0x82,0x21,
    0x00,0xA2,0x29,
    0x00,0xC3,0x31,
    0x00,0xE3,0x39,
    0x08,0x00,0x42,
    0x08,0x20,0x4A,
    0x08,0x41,0x52,
    0x08,0x61,0x5A,
    0x08,0x82,0x63,
    0x08,0xA2,0x6B,
    0x08,0xC3,0x73,
    0x08,0xE3,0x7B,
    0x10,0x00,0x84,
    0x10,0x20,0x8C,
    0x10,0x41,0x94,
    0x10,0x61,0x9C,
    0x10,0x82,0xA5,
    0x10,0xA2,0xAD,
    0x10,0xC3,0xB5,
    0x10,0xE3,0xBD,
    0x18,0x00,0xC6,
    0x18,0x20,0xCE,
    0x18,0x41,0xD6,
    0x18,0x61,0xDE,
    0x18,0x82,0xE7,
    0x18,0xA2,0xEF,
    0x18,0xC3,0xF7,
    0x18,0xE3,0xFF,
    0x21,0x04,0x00,
    0x21,0x24,0x08,
    0x21,0x45,0x10,
    0x21,0x65,0x18,
    0x21,0x86,0x21,
    0x21,0xA6,0x29,
    0x21,0xC7,0x31,
    0x21,0xE7,0x39,
    0x29,0x04,0x42,
    0x29,0x24,0x4A,
    0x29,0x45,0x52,
    0x29,0x65,0x5A,
    0x29,0x86,0x63,
    0x29,0xA6,0x6B,
    0x29,0xC7,0x73,
    0x29,0xE7,0x7B,
    0x31,0x04,0x84,
    0x31,0x24,0x8C,
    0x31,0x45,0x94,
    0x31,0x65,0x9C,
    0x31,0x86,0xA5,
    0x31,0xA6,0xAD,
    0x31,0xC7,0xB5,
    0x31,0xE7,0xBD,
    0x39,0x04,0xC6,
    0x39,0x24,0xCE,
    0x39,0x45,0xD6,
    0x39,0x65,0xDE,
    0x39,0x86,0xE7,
    0x39,0xA6,0xEF,
    0x39,0xC7,0xF7,
    0x39,0xE7,0xFF,
    0x42,0x08,0x00,
    0x42,0x28,0x08,
    0x42,0x49,0x10,
    0x42,0x69,0x18,
    0x42,0x8A,0x21,
    0x42,0xAA,0x29,
    0x42,0xCB,0x31,
    0x42,0xEB,0x39,
    0x4A,0x08,0x42,
    0x4A,0x28,0x4A,
    0x4A,0x49,0x52,
    0x4A,0x69,0x5A,
    0x4A,0x8A,0x63,
    0x4A,0xAA,0x6B,
    0x4A,0xCB,0x73,
    0x4A,0xEB,0x7B,
    0x52,0x08,0x84,
    0x52,0x28,0x8C,
    0x52,0x49,0x94,
    0x52,0x69,0x9C,
    0x52,0x8A,0xA5,
    0x52,0xAA,0xAD,
    0x52,0xCB,0xB5,
    0x52,0xEB,0xBD,
    0x5A,0x08,0xC6,
    0x5A,0x28,0xCE,
    0x5A,0x49,0xD6,
    0x5A,0x69,0xDE,
    0x5A,0x8A,0xE7,
    0x5A,0xAA,0xEF,
    0x5A,0xCB,0xF7,
    0x5A,0xEB,0xFF,
    0x63,0x0C,0x00,
    0x63,0x2C,0x08,
    0x63,0x4D,0x10,
    0x63,0x6D,0x18,
    0x63,0x8E,0x21,
    0x63,0xAE,0x29,
    0x63,0xCF,0x31,
    0x63,0xEF,0x39,
    0x6B,0x0C,0x42,
    0x6B,0x2C,0x4A,
    0x6B,0x4D,0x52,
    0x6B,0x6D,0x5A,
    0x6B,0x8E,0x63,
    0x6B,0xAE,0x6B,
    0x6B,0xCF,0x73,
    0x6B,0xEF,0x7B,
    0x73,0x0C,0x84,
    0x73,0x2C,0x8C,
    0x73,0x4D,0x94,
    0x73,0x6D,0x9C,
    0x73,0x8E,0xA5,
    0x73,0xAE,0xAD,
    0x73,0xCF,0xB5,
    0x73,0xEF,0xBD,
    0x7B,0x0C,0xC6,
    0x7B,0x2C,0xCE,
    0x7B,0x4D,0xD6,
    0x7B,0x6D,0xDE,
    0x7B,0x8E,0xE7,
    0x7B,0xAE,0xEF,
    0x7B,0xCF,0xF7,
    0x7B,0xEF,0xFF,
    0x84,0x10,0x00,
    0x84,0x30,0x08,
    0x84,0x51,0x10,
    0x84,0x71,0x18,
    0x84,0x92,0x21,
    0x84,0xB2,0x29,
    0x84,0xD3,0x31,
    0x84,0xF3,0x39,
    0x8C,0x10,0x42,
    0x8C,0x30,0x4A,
    0x8C,0x51,0x52,
    0x8C,0x71,0x5A,
    0x8C,0x92,0x63,
    0x8C,0xB2,0x6B,
    0x8C,0xD3,0x73,
    0x8C,0xF3,0x7B,
    0x94,0x10,0x84,
    0x94,0x30,0x8C,
    0x94,0x51,0x94,
    0x94,0x71,0x9C,
    0x94,0x92,0xA5,
    0x94,0xB2,0xAD,
    0x94,0xD3,0xB5,
    0x94,0xF3,0xBD,
    0x9C,0x10,0xC6,
    0x9C,0x30,0xCE,
    0x9C,0x51,0xD6,
    0x9C,0x71,0xDE,
    0x9C,0x92,0xE7,
    0x9C,0xB2,0xEF,
    0x9C,0xD3,0xF7,
    0x9C,0xF3,0xFF,
    0xA5,0x14,0x00,
    0xA5,0x34,0x08,
    0xA5,0x55,0x10,
    0xA5,0x75,0x18,
    0xA5,0x96,0x21,
    0xA5,0xB6,0x29,
    0xA5,0xD7,0x31,
    0xA5,0xF7,0x39,
    0xAD,0x14,0x42,
    0xAD,0x34,0x4A,
    0xAD,0x55,0x52,
    0xAD,0x75,0x5A,
    0xAD,0x96,0x63,
    0xAD,0xB6,0x6B,
    0xAD,0xD7,0x73,
    0xAD,0xF7,0x7B,
    0xB5,0x14,0x84,
    0xB5,0x34,0x8C,
    0xB5,0x55,0x94,
    0xB5,0x75,0x9C,
    0xB5,0x96,0xA5,
    0xB5,0xB6,0xAD,
    0xB5,0xD7,0xB5,
    0xB5,0xF7,0xBD,
    0xBD,0x14,0xC6,
    0xBD,0x34,0xCE,
    0xBD,0x55,0xD6,
    0xBD,0x75,0xDE,
    0xBD,0x96,0xE7,
    0xBD,0xB6,0xEF,
    0xBD,0xD7,0xF7,
    0xBD,0xF7,0xFF,
    0xC6,0x18,0x00,
    0xC6,0x38,0x08,
    0xC6,0x59,0x10,
    0xC6,0x79,0x18,
    0xC6,0x9A,0x21,
    0xC6,0xBA,0x29,
    0xC6,0xDB,0x31,
    0xC6,0xFB,0x39,
    0xCE,0x18,0x42,
    0xCE,0x38,0x4A,
    0xCE,0x59,0x52,
    0xCE,0x79,0x5A,
    0xCE,0x9A,0x63,
    0xCE,0xBA,0x6B,
    0xCE,0xDB,0x73,
    0xCE,0xFB,0x7B,
    0xD6,0x18,0x84,
    0xD6,0x38,0x8C,
    0xD6,0x59,0x94,
    0xD6,0x79,0x9C,
    0xD6,0x9A,0xA5,
    0xD6,0xBA,0xAD,
    0xD6,0xDB,0xB5,
    0xD6,0xFB,0xBD,
    0xDE,0x18,0xC6,
    0xDE,0x38,0xCE,
    0xDE,0x59,0xD6,
    0xDE,0x79,0xDE,
    0xDE,0x9A,0xE7,
    0xDE,0xBA,0xEF,
    0xDE,0xDB,0xF7,
    0xDE,0xFB,0xFF,
    0xE7,0x1C,0x00,
    0xE7,0x3C,0x08,
    0xE7,0x5D,0x10,
    0xE7,0x7D,0x18,
    0xE7,0x9E,0x21,
    0xE7,0xBE,0x29,
    0xE7,0xDF,0x31,
    0xE7,0xFF,0x39,
    0xEF,0x1C,0x42,
    0xEF,0x3C,0x4A,
    0xEF,0x5D,0x52,
    0xEF,0x7D,0x5A,
    0xEF,0x9E,0x63,
    0xEF,0xBE,0x6B,
    0xEF,0xDF,0x73,
    0xEF,0xFF,0x7B,
    0xF7,0x1C,0x84,
    0xF7,0x3C,0x8C,
    0xF7,0x5D,0x94,
    0xF7,0x7D,0x9C,
    0xF7,0x9E,0xA5,
    0xF7,0xBE,0xAD,
    0xF7,0xDF,0xB5,
    0xF7,0xFF,0xBD,
    0xFF,0x1C,0xC6,
    0xFF,0x3C,0xCE,
    0xFF,0x5D,0xD6,
    0xFF,0x7D,0xDE,
    0xFF,0x9E,0xE7,
    0xFF,0xBE,0xEF,
    0xFF,0xDF,0xF7,
    0xFF,0xFF,0xFF,
};

static uint8_t palette_rgb332[] =
{
    0x00,0x00,0x00,
    0x00,0x00,0x68,
    0x00,0x00,0xB7,
    0x00,0x00,0xFF,
    0x33,0x00,0x00,
    0x33,0x00,0x68,
    0x33,0x00,0xB7,
    0x33,0x00,0xFF,
    0x5C,0x00,0x00,
    0x5C,0x00,0x68,
    0x5C,0x00,0xB7,
    0x5C,0x00,0xFF,
    0x7F,0x00,0x00,
    0x7F,0x00,0x68,
    0x7F,0x00,0xB7,
    0x7F,0x00,0xFF,
    0xA2,0x00,0x00,
    0xA2,0x00,0x68,
    0xA2,0x00,0xB7,
    0xA2,0x00,0xFF,
    0xC1,0x00,0x00,
    0xC1,0x00,0x68,
    0xC1,0x00,0xB7,
    0xC1,0x00,0xFF,
    0xE1,0x00,0x00,
    0xE1,0x00,0x68,
    0xE1,0x00,0xB7,
    0xE1,0x00,0xFF,
    0xFF,0x00,0x00,
    0xFF,0x00,0x68,
    0xFF,0x00,0xB7,
    0xFF,0x00,0xFF,
    0x00,0x33,0x00,
    0x00,0x33,0x68,
    0x00,0x33,0xB7,
    0x00,0x33,0xFF,
    0x33,0x33,0x00,
    0x33,0x33,0x68,
    0x33,0x33,0xB7,
    0x33,0x33,0xFF,
    0x5C,0x33,0x00,
    0x5C,0x33,0x68,
    0x5C,0x33,0xB7,
    0x5C,0x33,0xFF,
    0x7F,0x33,0x00,
    0x7F,0x33,0x68,
    0x7F,0x33,0xB7,
    0x7F,0x33,0xFF,
    0xA2,0x33,0x00,
    0xA2,0x33,0x68,
    0xA2,0x33,0xB7,
    0xA2,0x33,0xFF,
    0xC1,0x33,0x00,
    0xC1,0x33,0x68,
    0xC1,0x33,0xB7,
    0xC1,0x33,0xFF,
    0xE1,0x33,0x00,
    0xE1,0x33,0x68,
    0xE1,0x33,0xB7,
    0xE1,0x33,0xFF,
    0xFF,0x33,0x00,
    0xFF,0x33,0x68,
    0xFF,0x33,0xB7,
    0xFF,0x33,0xFF,
    0x00,0x5C,0x00,
    0x00,0x5C,0x68,
    0x00,0x5C,0xB7,
    0x00,0x5C,0xFF,
    0x33,0x5C,0x00,
    0x33,0x5C,0x68,
    0x33,0x5C,0xB7,
    0x33,0x5C,0xFF,
    0x5C,0x5C,0x00,
    0x5C,0x5C,0x68,
    0x5C,0x5C,0xB7,
    0x5C,0x5C,0xFF,
    0x7F,0x5C,0x00,
    0x7F,0x5C,0x68,
    0x7F,0x5C,0xB7,
    0x7F,0x5C,0xFF,
    0xA2,0x5C,0x00,
    0xA2,0x5C,0x68,
    0xA2,0x5C,0xB7,
    0xA2,0x5C,0xFF,
    0xC1,0x5C,0x00,
    0xC1,0x5C,0x68,
    0xC1,0x5C,0xB7,
    0xC1,0x5C,0xFF,
    0xE1,0x5C,0x00,
    0xE1,0x5C,0x68,
    0xE1,0x5C,0xB7,
    0xE1,0x5C,0xFF,
    0xFF,0x5C,0x00,
    0xFF,0x5C,0x68,
    0xFF,0x5C,0xB7,
    0xFF,0x5C,0xFF,
    0x00,0x7F,0x00,
    0x00,0x7F,0x68,
    0x00,0x7F,0xB7,
    0x00,0x7F,0xFF,
    0x33,0x7F,0x00,
    0x33,0x7F,0x68,
    0x33,0x7F,0xB7,
    0x33,0x7F,0xFF,
    0x5C,0x7F,0x00,
    0x5C,0x7F,0x68,
    0x5C,0x7F,0xB7,
    0x5C,0x7F,0xFF,
    0x7F,0x7F,0x00,
    0x7F,0x7F,0x68,
    0x7F,0x7F,0xB7,
    0x7F,0x7F,0xFF,
    0xA2,0x7F,0x00,
    0xA2,0x7F,0x68,
    0xA2,0x7F,0xB7,
    0xA2,0x7F,0xFF,
    0xC1,0x7F,0x00,
    0xC1,0x7F,0x68,
    0xC1,0x7F,0xB7,
    0xC1,0x7F,0xFF,
    0xE1,0x7F,0x00,
    0xE1,0x7F,0x68,
    0xE1,0x7F,0xB7,
    0xE1,0x7F,0xFF,
    0xFF,0x7F,0x00,
    0xFF,0x7F,0x68,
    0xFF,0x7F,0xB7,
    0xFF,0x7F,0xFF,
    0x00,0xA2,0x00,
    0x00,0xA2,0x68,
    0x00,0xA2,0xB7,
    0x00,0xA2,0xFF,
    0x33,0xA2,0x00,
    0x33,0xA2,0x68,
    0x33,0xA2,0xB7,
    0x33,0xA2,0xFF,
    0x5C,0xA2,0x00,
    0x5C,0xA2,0x68,
    0x5C,0xA2,0xB7,
    0x5C,0xA2,0xFF,
    0x7F,0xA2,0x00,
    0x7F,0xA2,0x68,
    0x7F,0xA2,0xB7,
    0x7F,0xA2,0xFF,
    0xA2,0xA2,0x00,
    0xA2,0xA2,0x68,
    0xA2,0xA2,0xB7,
    0xA2,0xA2,0xFF,
    0xC1,0xA2,0x00,
    0xC1,0xA2,0x68,
    0xC1,0xA2,0xB7,
    0xC1,0xA2,0xFF,
    0xE1,0xA2,0x00,
    0xE1,0xA2,0x68,
    0xE1,0xA2,0xB7,
    0xE1,0xA2,0xFF,
    0xFF,0xA2,0x00,
    0xFF,0xA2,0x68,
    0xFF,0xA2,0xB7,
    0xFF,0xA2,0xFF,
    0x00,0xC1,0x00,
    0x00,0xC1,0x68,
    0x00,0xC1,0xB7,
    0x00,0xC1,0xFF,
    0x33,0xC1,0x00,
    0x33,0xC1,0x68,
    0x33,0xC1,0xB7,
    0x33,0xC1,0xFF,
    0x5C,0xC1,0x00,
    0x5C,0xC1,0x68,
    0x5C,0xC1,0xB7,
    0x5C,0xC1,0xFF,
    0x7F,0xC1,0x00,
    0x7F,0xC1,0x68,
    0x7F,0xC1,0xB7,
    0x7F,0xC1,0xFF,
    0xA2,0xC1,0x00,
    0xA2,0xC1,0x68,
    0xA2,0xC1,0xB7,
    0xA2,0xC1,0xFF,
    0xC1,0xC1,0x00,
    0xC1,0xC1,0x68,
    0xC1,0xC1,0xB7,
    0xC1,0xC1,0xFF,
    0xE1,0xC1,0x00,
    0xE1,0xC1,0x68,
    0xE1,0xC1,0xB7,
    0xE1,0xC1,0xFF,
    0xFF,0xC1,0x00,
    0xFF,0xC1,0x68,
    0xFF,0xC1,0xB7,
    0xFF,0xC1,0xFF,
    0x00,0xE1,0x00,
    0x20,0xE1,0x68,
    0x00,0xE1,0xB7,
    0x00,0xE1,0xFF,
    0x33,0xE1,0x00,
    0x33,0xE1,0x68,
    0x33,0xE1,0xB7,
    0x33,0xE1,0xFF,
    0x5C,0xE1,0x00,
    0x5C,0xE1,0x68,
    0x5C,0xE1,0xB7,
    0x5C,0xE1,0xFF,
    0x7F,0xE1,0x00,
    0x7F,0xE1,0x68,
    0x7F,0xE1,0xB7,
    0x7F,0xE1,0xFF,
    0xA2,0xE1,0x00,
    0xA2,0xE1,0x68,
    0xA2,0xE1,0xB7,
    0xA2,0xE1,0xFF,
    0xC1,0xE1,0x00,
    0xC1,0xE1,0x68,
    0xC1,0xE1,0xB7,
    0xC1,0xE1,0xFF,
    0xE1,0xE1,0x00,
    0xE1,0xE1,0x68,
    0xE1,0xE1,0xB7,
    0xE1,0xE1,0xFF,
    0xFF,0xE1,0x00,
    0xFF,0xE1,0x68,
    0xFF,0xE1,0xB7,
    0xFF,0xE1,0xFF,
    0x00,0xFF,0x00,
    0x00,0xFF,0x68,
    0x00,0xFF,0xB7,
    0x00,0xFF,0xFF,
    0x33,0xFF,0x00,
    0x33,0xFF,0x68,
    0x33,0xFF,0xB7,
    0x33,0xFF,0xFF,
    0x5C,0xFF,0x00,
    0x5C,0xFF,0x68,
    0x5C,0xFF,0xB7,
    0x5C,0xFF,0xFF,
    0x7F,0xFF,0x00,
    0x7F,0xFF,0x68,
    0x7F,0xFF,0xB7,
    0x7F,0xFF,0xFF,
    0xA2,0xFF,0x00,
    0xA2,0xFF,0x68,
    0xA2,0xFF,0xB7,
    0xA2,0xFF,0xFF,
    0xC1,0xFF,0x00,
    0xC1,0xFF,0x68,
    0xC1,0xFF,0xB7,
    0xC1,0xFF,0xFF,
    0xE1,0xFF,0x00,
    0xE1,0xFF,0x68,
    0xE1,0xFF,0xB7,
    0xE1,0xFF,0xFF,
    0xFF,0xFF,0x00,
    0xFF,0xFF,0x68,
    0xFF,0xFF,0xB7,
    0xFF,0xFF,0xFF,
};
