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
#include "log.h"

#include <stdlib.h>
#include <string.h>

struct tileset_group *tileset_group_alloc(void)
{
    struct tileset_group *tileset_group;

    tileset_group = malloc(sizeof(struct tileset_group));
    if (tileset_group == NULL)
    {
        LOG_ERROR("Out of memory\n");
        return NULL;
    }

    tileset_group->tilesets = NULL;
    tileset_group->nr_tilesets = 0;
    tileset_group->tile_height = 16;
    tileset_group->tile_width = 16;
    tileset_group->p_table = true;

    return tileset_group;
}

void tileset_free(struct tileset *tileset)
{
    uint32_t i;

    for (i = 0; i < tileset->nr_tiles; ++i)
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

void tileset_group_free(struct tileset_group *tileset_group)
{
    uint32_t i;

    if (tileset_group == NULL)
    {
        return;
    }

    for (i = 0; i < tileset_group->nr_tilesets; ++i)
    {
        if (tileset_group->tilesets != NULL)
        {
            tileset_free(&tileset_group->tilesets[i]);
        }
    }

    free(tileset_group->tilesets);
    tileset_group->tilesets = NULL;

    tileset_group->nr_tilesets = 0;
}
