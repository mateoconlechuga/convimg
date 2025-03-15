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

#include "appvar.h"
#include "clean.h"
#include "log.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static uint32_t appvar_checksum(const uint8_t *arr, size_t size)
{
    uint32_t checksum = 0;
    size_t i;

    for (i = 0; i < size; ++i)
    {
        checksum += arr[APPVAR_VAR_HEADER_POS + i];
        checksum &= 0xffff;
    }

    return checksum;
}

int appvar_write(struct appvar *a, const char *path)
{
    static const uint8_t file_header[11] =
        { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A,0x00 };
    static uint8_t output[APPVAR_MAX_FILE_SIZE];
    uint32_t checksum;
    FILE *fdv = NULL;
    size_t file_size;
    size_t data_size;
    size_t varb_size;
    size_t var_size;
    int write_error = -1;

    LOG_INFO(" - Writing \'%s\'\n", path);

    fdv = clean_fopen(path, "wb");
    if (fdv == NULL)
    {
        LOG_ERROR("Could not open file: %s\n", strerror(errno));
        goto error;
    }

    memset(output, 0, sizeof output);

    file_size = a->size + APPVAR_DATA_POS + APPVAR_CHECKSUM_LEN;
    data_size = a->size + APPVAR_VAR_HEADER_LEN + APPVAR_VARB_SIZE_LEN;
    var_size = a->size + APPVAR_VARB_SIZE_LEN;
    varb_size = a->size;

    memcpy(output + APPVAR_FILE_HEADER_POS, file_header, sizeof file_header);
    memcpy(output + APPVAR_COMMENT_POS, a->comment, APPVAR_MAX_COMMENT_SIZE);
    memcpy(output + APPVAR_NAME_POS, a->name, APPVAR_MAX_NAME_SIZE);
    memcpy(output + APPVAR_DATA_POS, a->data, varb_size);


    output[APPVAR_VAR_HEADER_POS] = APPVAR_MAGIC;
    output[APPVAR_TYPE_POS] = APPVAR_TYPE_FLAG;
    output[APPVAR_ARCHIVE_POS] = APPVAR_ARCHIVE_FLAG;

    output[APPVAR_DATA_SIZE_POS + 0] = (data_size >> 0) & 0xff;
    output[APPVAR_DATA_SIZE_POS + 1] = (data_size >> 8) & 0xff;

    output[APPVAR_VARB_SIZE_POS + 0] = (varb_size >> 0) & 0xff;
    output[APPVAR_VARB_SIZE_POS + 1] = (varb_size >> 8) & 0xff;

    output[APPVAR_VAR_SIZE0_POS + 0] = (var_size >> 0) & 0xff;
    output[APPVAR_VAR_SIZE0_POS + 1] = (var_size >> 8) & 0xff;
    output[APPVAR_VAR_SIZE1_POS + 0] = (var_size >> 0) & 0xff;
    output[APPVAR_VAR_SIZE1_POS + 1] = (var_size >> 8) & 0xff;

    checksum = appvar_checksum(output, data_size);

    output[APPVAR_DATA_POS + varb_size + 0] = (checksum >> 0) & 0xff;
    output[APPVAR_DATA_POS + varb_size + 1] = (checksum >> 8) & 0xff;

    write_error = fwrite(output, file_size, 1, fdv) == 1 ? 0 : -1;

    if (write_error)
    {
        if (remove(path))
        {
            LOG_ERROR("Could not remove file: %s\n", strerror(errno));
        }
    }

error:

    if (fdv != NULL)
    {
        fclose(fdv);
    }

    return write_error;
}
