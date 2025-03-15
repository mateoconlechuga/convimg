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

#ifndef APPVAR_H
#define APPVAR_H

#include "compress.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APPVAR_MAX_FILE_SIZE (64 * 1024 + 300)
#define APPVAR_MAX_DATA_SIZE (65505)
#define APPVAR_MAX_BEFORE_COMPRESSION_SIZE (65505*4)
#define APPVAR_MAX_COMMENT_SIZE 42
#define APPVAR_MAX_NAME_SIZE 8

#define APPVAR_TYPE_FLAG 21
#define APPVAR_ARCHIVE_FLAG 128

#define APPVAR_CHECKSUM_LEN 2
#define APPVAR_VARB_SIZE_LEN 2
#define APPVAR_VAR_HEADER_LEN 17
#define APPVAR_FILE_HEADER_LEN 55

#define APPVAR_FILE_HEADER_POS 0x00
#define APPVAR_COMMENT_POS 0x0B
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
    APPVAR_SOURCE_NONE,
    APPVAR_SOURCE_C,
    APPVAR_SOURCE_ICE,
    APPVAR_SOURCE_ASM,
} appvar_source_t;

struct appvar
{
    char name[APPVAR_MAX_NAME_SIZE + 1];
    char *header;
    uint8_t *data;
    bool archived;
    bool init;
    bool lut;
    uint32_t size;
    uint32_t uncompressed_size;
    uint32_t entry_size;
    uint32_t nr_entries;
    uint32_t total_entries;
    uint32_t header_size;
    uint32_t data_offset;
    char comment[APPVAR_MAX_COMMENT_SIZE + 1];
    appvar_source_t source;
    compress_mode_t compress;
};

int appvar_write(struct appvar *a, const char *path);

#ifdef __cplusplus
}
#endif

#endif
