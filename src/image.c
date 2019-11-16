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

/*
 * Loads an image to its data array.
 */
int image_load(image_t *image)
{
    int channels;
    image->data = (uint8_t *)stbi_load(image->name,
                                       &image->width,
                                       &image->height,
                                       &channels,
                                       STBI_rgb_alpha);

	image->size = image->width * image->height;

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
    free(image->data);
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
	    LL_ERROR("Failed to create image attributes \'%s\'\n", image->name);
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
	    LL_ERROR("Failed to create image \'%s\'\n", image->name);
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
	    LL_ERROR("Failed to quantize image \'%s\'\n", image->name);
	    liq_image_destroy(liqimage);
	    liq_attr_destroy(liqattr);
	    return 1;
	}

	data = malloc(image->size);
	if (data == NULL)
	{
	    LL_ERROR("Failed to allocate image memory \'%s\'\n", image->name);
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
