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

#include "tileset.h"

#include <stdlib.h>
#include <string.h>

/*
 * Allocates storage for each tile.
 */
int tileset_alloc_tiles(tileset_t *tileset)
{
	int i;
	int tileSize = tileset->tileWidth * tileset->tileHeight;

	tileset->tiles =
		malloc(tileset->numTiles * sizeof(tileset_tile_t));
	if (tileset->tiles == NULL)
	{
		return 1;
	}

	for (i = 0; i < tileset->numTiles; ++i)
	{
		tileset->tiles[i].data = malloc(tileSize);
		if (tileset->tiles[i].data == NULL)
		{
			return 1;
		}

		tileset->tiles[i].size = tileSize;
	}

	return 0;
}

/*
 * Frees an allocated tileset.
 */
void tileset_free(tileset_t *tileset)
{
	int i;

	if (tileset == NULL || tileset->tiles == NULL)
	{
		return;
	}

	for (i = 0; i < tileset->numTiles; ++i)
	{
		free(tileset->tiles[i].data);
		tileset->tiles[i].data = NULL;
	}

	free(tileset->tiles);
	tileset->tiles = NULL;
}
