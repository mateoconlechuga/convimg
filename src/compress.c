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

#include "compress.h"
#include "log.h"

#include "deps/zx7/zx7.h"

#include <string.h>

static int compress_zx7(unsigned char **arr, size_t *size)
{
    long delta;
    Optimal *opt;
    unsigned char *compressed;

    if (size == NULL || arr == NULL)
    {
        LL_DEBUG("invalid param in %s.", __func__);
        return 1;
    }

    opt = optimize(*arr, *size);
    compressed = compress(opt, *arr, *size, size, &delta);
    free(*arr);
    *arr = compressed;

    if (delta < 0)
    {
        /* send warning to user? */
    }

    free(opt);
    return 0;
}

/*
 * Compress output array before writing to output.
 * Returns compressed array, or NULL if error.
 */
int compress_array(unsigned char **arr, size_t *size, compress_t mode)
{
    int ret = 0;

    switch (mode)
    {
        case COMPRESS_NONE:
            ret = 0;
            break;

        case COMPRESS_ZX7:
            ret = compress_zx7(arr, size);
            break;

        case COMPRESS_INVALID:
            ret = 1;
            break;
    }

    return ret;
}
