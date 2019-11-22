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

#include "image.h"
#include "palette.h"
#include "log.h"

#include "deps/libimagequant/libimagequant.h"

#define STB_IMAGE_IMPLEMENTATION
#include "deps/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "deps/stb/stb_image_write.h"

/*
 * Loads an image to its data array.
 */
int image_load(image_t *image)
{
    int channels;
    image->data = (uint8_t *)stbi_load(image->path,
                                       &image->width,
                                       &image->height,
                                       &channels,
                                       STBI_rgb_alpha);

    image->size = image->width * image->height;
    image->compressed = false;

    return image->data == NULL ? 1 : 0;
}

/*
 * Frees a previously loaded image
 */
void image_free(image_t *image)
{
    if( image == NULL )
    {
        return;
    }

    free(image->name);
    free(image->path);
    free(image->data);
}

/*
 * Adds width and height to image.
 */
int image_add_width_and_height(image_t *image)
{
    if (image == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        return 1;
    }

    image->data = realloc(image->data, image->size + WIDTH_HEIGHT_SIZE);
    if (image->data == NULL)
    {
        return 1;
    }

    memmove(image->data + WIDTH_HEIGHT_SIZE, image->data, image->size);

    image->data[0] = image->width;
    image->data[1] = image->height;

    image->size += WIDTH_HEIGHT_SIZE;

    return 0;
}

/*
 * Converts image to RLET encoded.
 */
int image_rlet(image_t *image, int tIndex)
{
    uint8_t *newData = calloc(image->width * image->height * 2, sizeof(uint8_t));
    int newSize = 0;
    int i;

    if (newData == NULL)
    {
        LL_DEBUG("Memory error in %s", __func__);
        return 1;
    }

    if (tIndex < 0)
    {
        LL_ERROR("Transparent color index not specified for RLET mode.");
        return 1;
    }

    for (i = 0; i < image->height; i++)
    {
        int offset = i * image->width;
        int left = image->width;

        while (left)
        {
            int o, t;

            t = o = 0;
            while (t < left && tIndex == image->data[t + offset])
            {
                t++;
            }

            newData[newSize] = t;
            newSize++;

            if ((left -= t))
            {
                uint8_t *opaqueLen = &newData[newSize];
                newSize++;

                while (o < left && tIndex != image->data[t + o + offset])
                {
                    newData[newSize] = image->data[t + o + offset];
                    newSize++;
                    o++;
                }

                *opaqueLen = o;
                left -= o;
            }

            offset += o + t;
        }
    }

    free(image->data);
    image->data = newData;
    image->size = newSize;

    return 0;
}

/*
 * Removes omited indicies from the converted data.
 * Reallocs array as needed.
 */
int image_remove_omits(image_t *image, int *omitIndices, int numOmitIndices)
{
    int i, j;
    int newSize = 0;
    uint8_t *newData;

    if (numOmitIndices == 0)
    {
        return 0;
    }

    newData = malloc(image->size);
    if (newData == NULL)
    {
        return 1;
    }

    for (i = 0; i < image->size; ++i)
    {
        for (j = 0; j < numOmitIndices; ++j)
        {
            if (image->data[i] == omitIndices[j])
            {
                goto nextbyte;
            }
        }

        newData[newSize] = image->data[i];
        newSize++;

nextbyte:
        continue;
    }

    free(image->data);
    image->data = newData;
    image->size = newSize;

    return 0;
}

/*
 * Compresses data (includes width and height if they exist).
 * Reallocs array as needed.
 */
int image_compress(image_t *image, compress_t compress)
{
    size_t newSize;
    int ret = 0;

    if (image == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        return 1;
    }

    if (compress == COMPRESS_NONE)
    {
        return 0;
    }

    newSize = image->size;
    ret = compress_array(&image->data, &newSize, compress);
    image->size = newSize;

    return ret;
}

/*
 * Quantizes an image against a palette.
 */
int image_quantize(image_t *image, palette_t *palette)
{
    int j;
    liq_image *liqimage = NULL;
    liq_result *liqresult = NULL;
    liq_attr *liqattr = NULL;
    uint8_t *data = NULL;

    liqattr = liq_attr_create();
    if (liqattr == NULL)
    {
        LL_ERROR("Failed to create image attributes \'%s\'\n", image->path);
        return 1;
    }

    liq_set_speed(liqattr, 10);
    liq_set_max_colors(liqattr, palette->numEntries);
    liqimage = liq_image_create_rgba(liqattr,
                                     image->data,
                                     image->width,
                                     image->height,
                                     0);
    if (liqimage == NULL)
    {
        LL_ERROR("Failed to create image \'%s\'\n", image->path);
        liq_attr_destroy(liqattr);
        return 1;
    }

    for (j = 0; j < palette->numEntries; j++)
    {
        liq_image_add_fixed_color(liqimage, palette->entries[j].color.rgb);
    }

    liqresult = liq_quantize_image(liqattr, liqimage);
    if (liqresult == NULL)
    {
        LL_ERROR("Failed to quantize image \'%s\'\n", image->path);
        liq_image_destroy(liqimage);
        liq_attr_destroy(liqattr);
        return 1;
    }

    data = malloc(image->size);
    if (data == NULL)
    {
        LL_ERROR("Failed to allocate image memory \'%s\'\n", image->path);
        liq_result_destroy(liqresult);
        liq_image_destroy(liqimage);
        liq_attr_destroy(liqattr);
        return 1;
    }

    liq_write_remapped_image(liqresult, liqimage, data, image->size);
    free(image->data);
    image->data = data;

    liq_result_destroy(liqresult);
    liq_image_destroy(liqimage);
    liq_attr_destroy(liqattr);

    return 0;
}
