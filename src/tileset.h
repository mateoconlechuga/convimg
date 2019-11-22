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

#ifndef TILESET_H
#define TILESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "image.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint8_t *data;
    int size;
} tileset_tile_t;

typedef struct
{
    tileset_tile_t *tiles;
    int numTiles;
    image_t image;

    /* duplicate parameters from parent */
    int tileHeight;
    int tileWidth;
    bool pTable;

    /* set by convert */
    bool compressed;

    /* set by output */
    int appvarIndex;
    char *directory;
} tileset_t;

typedef struct
{
    tileset_t *tilesets;
    int numTilesets;
    int tileHeight;
    int tileWidth;
    bool pTable;
} tileset_group_t;

tileset_t *tileset_alloc(void);
tileset_group_t *tileset_group_alloc(void);
int tileset_alloc_tiles(tileset_t *tileset);
void tileset_free(tileset_t *tileset);
void tileset_group_free(tileset_group_t *tilesetGroup);

#ifdef __cplusplus
}
#endif

#endif
