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

#ifndef APPVAR_H
#define APPVAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "compress.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define APPVAR_MAX_FILE_SIZE (64 * 1024 + 300)
#define APPVAR_MAX_DATA_SIZE (64 * 1024 - 300)

#define APPVAR_TYPE_FLAG 21
#define APPVAR_ARCHIVE_FLAG 128

#define APPVAR_CHECKSUM_LEN 2
#define APPVAR_VARB_SIZE_LEN 2
#define APPVAR_VAR_HEADER_LEN 17
#define APPVAR_FILE_HEADER_LEN 55

#define APPVAR_FILE_HEADER_POS 0x00
#define APPVAR_DATA_SIZE_POS 0x35
#define APPVAR_VAR_HEADER_POS 0x37
#define APPVAR_VAR_SIZE0_POS 0x39
#define APPVAR_TYPE_POS 0x3b
#define APPVAR_NAME_POS 0x3c
#define APPVAR_ARCHIVE_POS 0x45
#define APPVAR_VAR_SIZE1_POS 0x46
#define APPVAR_VARB_SIZE_POS 0x48
#define APPVAR_DATA_POS 0x4a

#define APPVAR_MAGIC 0x0d

typedef enum
{
    APPVAR_SOURCE_C,
    APPVAR_SOURCE_ICE,
} appvar_source_t;

typedef struct
{
    char *name;
    bool archived;
    appvar_source_t source;
    bool init;
    compress_t compress;
    uint8_t *data;
    int size;
    int numEntries;

    /* set by output */
    char *directory;
} appvar_t;

int appvar_write(appvar_t *a, FILE *fdv);

#ifdef __cplusplus
}
#endif

#endif
