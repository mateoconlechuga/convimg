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
#include "deps/lz4/lib/lz4.h"

#include <string.h>

static uint8_t *compress_zx7(void *data, size_t *size)
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

    LOG_DEBUG("Compressed size: %u -> %u (zx7)\n", *size, new_size);

    *size = new_size;

    return compressed_data;
}

static void compress_zx0_progress(int amount)
{
    LOG_INFO(" - Compressing Data (%d%%)\n", amount * 10);
}

static uint8_t *compress_zx0(void *data, size_t *size)
{
    uint8_t *compressed_data;
    int orig_size;
    int new_size;
    int delta;

    if (size == NULL || data == NULL)
    {
        return NULL;
    }

    orig_size = *size;

    compressed_data = zx0_compress(data, orig_size, 0, 0, 1, &new_size, &delta, orig_size > 16384 ? compress_zx0_progress : NULL);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return NULL;
    }

    LOG_DEBUG("Compressed size: %u -> %u (zx0)\n", orig_size, new_size);

    *size = new_size;

    return compressed_data;
}

static uint8_t *compress_lz4(void *data, size_t *size)
{
    const char *input = data;
    uint8_t *compressed_data;
    int orig_size = *size;
    int new_size;

    if (size == NULL || input == NULL)
    {
        return NULL;
    }

    orig_size = *size;
    new_size = LZ4_compressBound(orig_size);
    compressed_data = malloc(new_size);
    if (compressed_data == NULL)
    {
        LOG_ERROR("Out of memory.\n");
        return NULL;
    }

    new_size = LZ4_compress_default(input, (char*)compressed_data, orig_size, new_size);
    if (!new_size)
    {
        free(compressed_data);
        LOG_ERROR("LZ4 compression failed.\n");
        return NULL;
    }

    LOG_DEBUG("Compressed size: %u -> %u (lz4)\n", orig_size, new_size);

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

        case COMPRESS_LZ4:
            return compress_lz4(data, size);

        default:
            return NULL;
    }
}
