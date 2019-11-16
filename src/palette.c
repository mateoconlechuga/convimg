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

#include "palette.h"
#include "image.h"
#include "log.h"

#include "deps/libimagequant/libimagequant.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Allocates palette structure.
 */
palette_t *palette_alloc(void)
{
    palette_t *palette = NULL;

    palette = malloc(sizeof(palette_t));
    if (palette == NULL)
    {
        return NULL;
    }

    palette->image = NULL;
    palette->numImages = 0;
    palette->maxEntries = PALETTE_MAX_ENTRIES;
    palette->numEntries = 0;
    palette->numFixedEntries = 0;
    palette->bpp = BPP_8;

    return palette;
}

/*
 * Adds a image file to a palette (does not load).
 */
int palette_add_image(palette_t *palette, const char *name)
{
    image_t *image;

    if (palette == NULL || name == NULL)
    {
        return 1;
    }

    palette->image =
        realloc(palette->image, (palette->numImages + 1) * sizeof(image_t));
    if (palette->image == NULL)
    {
        return 1;
    }

    image = &palette->image[palette->numImages];

    image->name = strdup(name);
    image->data = NULL;
    image->width = 0;
    image->height = 0;

    palette->numImages++;

    return 0;
}

/*
 * Frees an allocated palette.
 */
void palette_free(palette_t *palette)
{
    int i;

    if (palette == NULL)
    {
        return;
    }

    for (i = 0; i < palette->numImages; ++i)
    {
        free(palette->image[i].name);
        palette->image[i].name = NULL;
    }

    free(palette->image);
    palette->image = NULL;
}

/*
 * Reads all input images, and generates a palette for convert.
 */
int palette_generate(palette_t *palette)
{
    int i, j;
    liq_attr *attr = NULL;
    liq_histogram *hist = NULL;
    liq_result *liqresult = NULL;
    const liq_palette *liqpalette = NULL;
    liq_error liqerr;

    if (palette == NULL)
    {
        return 1;
    }

    LL_PRINT("[info] Generating palette \'%s\'\n", palette->name);

    attr = liq_attr_create();

    liq_set_max_colors(attr, palette->maxEntries);

    hist = liq_histogram_create(attr);

    for (i = 0; i < palette->numFixedEntries; ++i)
    {
        color_t *color = &palette->fixedEntries[i].color;

        color_convert(color, palette->mode);

        liq_histogram_add_fixed_color(hist, color->rgb, 0);
    }

    for (i = 0; i < palette->numImages; ++i)
    {
        image_t *image = &palette->image[i];
        liq_image *liqimage;

        LL_PRINT("[info] \'%s\' (%d of %d)\n",
            image->name,
            i + 1,
            palette->numImages);

        if( image_load(image) != 0 )
        {
            LL_ERROR("Failed to load image %s\n", image->name);
            liq_histogram_destroy(hist);
            liq_attr_destroy(attr);
            return 1;
        }

        liqimage = liq_image_create_rgba(attr,
                                         image->data,
                                         image->width,
                                         image->height,
                                         0);

        liq_histogram_add_image(hist, attr, liqimage);
        liq_image_destroy(liqimage);
        image_unload(image);
    }

    liqerr = liq_histogram_quantize(hist, attr, &liqresult);
    if (liqerr != LIQ_OK)
    {
        LL_ERROR("Failed to generate palette \'%s\'\n", palette->name);
        liq_histogram_destroy(hist);
        liq_attr_destroy(attr);
        return 1;
    }

    liqpalette = liq_get_palette(liqresult);

    palette->numEntries = liqpalette->count;

    for (i = 0; i < (int)liqpalette->count; ++i)
    {
        color_t *color = &palette->entries[i].color;

        color->rgb = liqpalette->entries[i];

        color_convert(color, palette->mode);
    }

    for (i = 0; i < (int)liqpalette->count; ++i)
    {
        color_t *ec = &palette->entries[i].color;

        ec->rgb = liqpalette->entries[i];

        color_convert(ec, palette->mode);

        palette->entries[i].index = i;
    }

    for (i = 0; i < palette->numFixedEntries; ++i)
    {
        color_t *fc = &palette->fixedEntries[i].color;

        for (j = 0; j < (int)liqpalette->count; ++j)
        {
            color_t *ec = &palette->entries[j].color;

            if( ec->target == fc->target )
            {
                color_t tmp = *ec;
                *ec = *fc;
                *fc = tmp;

                i = -1;
                break;
            }
        }
    }

    liq_result_destroy(liqresult);
    liq_histogram_destroy(hist);
    liq_attr_destroy(attr);

    return 0;
}
