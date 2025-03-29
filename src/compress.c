/*
 * Copyright 2017-2025 Matt "MateoConLechuga" Waltz
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

#include "compress.h"
#include "memory.h"
#include "log.h"

#include "deps/zx/zx7/zx7.h"
#include "deps/zx/zx0/zx0.h"

#include <string.h>

static uint8_t *compress_zx7(uint8_t *data, size_t *size)
{
    uint8_t *compressed_data;
    int new_size;
    long delta;

    if (size == NULL || data == NULL)
    {
        return NULL;
    }

    compressed_data = zx7_compress(data, *size, 0, &new_size, &delta);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return NULL;
    }

    LOG_DEBUG("Compressed size: %u -> %u\n", *size, new_size);

    *size = new_size;

    return compressed_data;
}

static void compress_zx0_progress(int amount)
{
    LOG_INFO(" - Compressing Data (%d%%)\n", amount * 10);
}

static uint8_t *compress_zx0(uint8_t *data, size_t *size)
{
    uint8_t *compressed_data;
    int orig_size = *size;
    int new_size;
    int delta;

    if (size == NULL || data == NULL)
    {
        return NULL;
    }

    compressed_data = zx0_compress(data, orig_size, 0, 0, 1, &new_size, &delta, orig_size > 16384 ? compress_zx0_progress : NULL);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return NULL;
    }

    LOG_DEBUG("Compressed size: %u -> %u\n", orig_size, new_size);

    *size = new_size;

    return compressed_data;
}

uint8_t *compress_array(uint8_t *data, size_t *size, compress_mode_t mode)
{
    switch (mode)
    {
        case COMPRESS_ZX7:
            return compress_zx7(data, size);

        case COMPRESS_ZX0:
            return compress_zx0(data, size);

        default:
            return NULL;
    }
}
