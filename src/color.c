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

#include "color.h"
#include "math.h"

#include <stdbool.h>

uint16_t color_to_565_rgb(const struct color *color)
{
    uint8_t r5 = round((int)color->r * 31.0 / 255.0);
    uint8_t g6 = round((int)color->g * 63.0 / 255.0);
    uint8_t b5 = round((int)color->b * 31.0 / 255.0);

    return (r5 << 11) | (g6 << 5) | b5;
}

uint16_t color_to_565_bgr(const struct color *color)
{
    uint8_t r5 = round((int)color->r * 31.0 / 255.0);
    uint8_t g6 = round((int)color->g * 63.0 / 255.0);
    uint8_t b5 = round((int)color->b * 31.0 / 255.0);

    return (b5 << 11) | (g6 << 5) | r5;
}

uint16_t color_to_1555_grgb(const struct color *color)
{
    uint8_t r5 = round((int)color->r * 31.0 / 255.0);
    uint8_t g6 = round((int)color->g * 63.0 / 255.0);
    uint8_t b5 = round((int)color->b * 31.0 / 255.0);

    return ((g6 & 1) << 15) | (r5 << 10) | ((g6 >> 1) << 5) | b5;
}

void color_normalize(struct color *color, color_format_t fmt)
{
    uint16_t tmp;

    switch (fmt)
    {
        case COLOR_1555_GRGB:
            tmp = color_to_1555_grgb(color);

            /* 1555 -> 888 */
            {
                uint8_t r5 = (tmp >> 10) & 31;
                uint8_t g6 = ((tmp >> 4) & 62) | (tmp >> 15);
                uint8_t b5 = tmp & 31;

                color->r = round((int)r5 * 255.0 / 31.0);
                color->g = round((int)g6 * 255.0 / 63.0);
                color->b = round((int)b5 * 255.0 / 31.0);
            }
            break;

        case COLOR_565_BGR:
        case COLOR_565_RGB:
            tmp = color_to_565_rgb(color);

            /* 565 -> 888 */
            {
                uint8_t r5 = (tmp >> 11) & 31;
                uint8_t g6 = ((tmp >> 5) & 62);
                uint8_t b5 = tmp & 31;

                color->r = round((int)r5 * 255.0 / 31.0);
                color->g = round((int)g6 * 255.0 / 63.0);
                color->b = round((int)b5 * 255.0 / 31.0);
            }
            break;

        default:
            break;
    }
}
