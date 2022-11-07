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

#include "tileset.h"
#include "memory.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

void tileset_free_tiles(struct tileset *tileset)
{
    for (uint32_t i = 0; i < tileset->nr_tiles; ++i)
    {
        if (tileset->tiles != NULL)
        {
            free(tileset->tiles[i].data);
            tileset->tiles[i].data = NULL;
        }
    }

    free(tileset->tiles);
    tileset->tiles = NULL;

    image_free(&tileset->image);
}

int tileset_alloc_tiles(struct tileset *tileset, uint32_t nr_tiles)
{
    tileset->tiles = memory_realloc_array(NULL, nr_tiles, sizeof(struct tileset_tile));
    if (tileset->tiles == NULL)
    {
        return -1;
    }

    for (uint32_t i = 0; i < nr_tiles; ++i)
    {
        uint32_t tile_dim = tileset->tile_width * tileset->tile_height;
        uint32_t tile_data_size = tile_dim * sizeof(uint32_t);
        uint8_t *tile_data;

        tile_data = memory_alloc(tile_data_size);
        if (tile_data == NULL)
        {
            return -1;
        }

        tileset->tiles[i].data_size = tile_data_size;
        tileset->tiles[i].data = tile_data;
    }

    tileset->nr_tiles = nr_tiles;

    return 0;
}
