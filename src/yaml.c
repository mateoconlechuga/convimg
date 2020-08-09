/*
 * Copyright 2017-2020 Matt "MateoConLechuga" Waltz
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

#include "yaml.h"
#include "strings.h"
#include "log.h"

#include "deps/libyaml/include/yaml.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

/*
 * Compares constant-length string to user string.
 */
static bool parse_str_cmp(const char *str, void *src)
{
    if (str == NULL || src == NULL)
    {
        return false;
    }

    return strncmp(str, src, strlen(str)) == 0;
}

/*
 * Computes boolean representation of string.
 */
static bool parse_str_bool(void *src)
{
    return parse_str_cmp("true", src);
}

/*
 * Prints mark error information for the user.
 */
static void parser_show_mark_error(yaml_mark_t mark)
{
    LL_ERROR("Problem is probably around line %" PRIuPTR ".", mark.line + 1);
}

/*
 * Prints error information for the user.
 */
static void parser_show_error(const char *problem, yaml_mark_t mark)
{
    LL_ERROR("Problem description: %s", problem);
    parser_show_mark_error(mark);
}

/*
 * Allocates a palette structure.
 */
static palette_t *parser_alloc_palette(yaml_file_t *data, void *name)
{
    palette_t *tmpPalette;
    int i;

    data->palettes =
        realloc(data->palettes, (data->numPalettes + 1) * sizeof(palette_t *));
    if (data->palettes == NULL)
    {
        return NULL;
    }

    LL_DEBUG("Allocating palette: %s", (char*)name);

    tmpPalette = palette_alloc();
    if (tmpPalette == NULL)
    {
        return NULL;
    }

    tmpPalette->name = strdup(name);

    for (i = 0; i < PALETTE_MAX_ENTRIES; ++i)
    {
        palette_entry_t *entry = &tmpPalette->entries[i];

        entry->valid = false;
        entry->exact = false;
        entry->color.rgb.r = 255;
        entry->color.rgb.g = 255;
        entry->color.rgb.b = 255;
        entry->color.rgb.a = 255;
        entry->origcolor = entry->color;
    }

    data->palettes[data->numPalettes] = tmpPalette;
    data->numPalettes++;

    return tmpPalette;
}

/*
 * Allocates convert structure.
 */
static convert_t *parser_alloc_convert(yaml_file_t *data, void *name)
{
    convert_t *tmpConvert;

    data->converts =
        realloc(data->converts, (data->numConverts + 1) * sizeof(convert_t *));
    if (data->converts == NULL)
    {
        return NULL;
    }

    LL_DEBUG("Allocating convert: %s", (char*)name);

    tmpConvert = convert_alloc();
    if (tmpConvert == NULL)
    {
        return NULL;
    }

    tmpConvert->name = strdup(name);

    data->converts[data->numConverts] = tmpConvert;
    data->numConverts++;

    return tmpConvert;
}

/*
 * Allocates output structure.
 */
static output_t *parser_alloc_output(yaml_file_t *data, void *type)
{
    output_t *tmpOutput;

    data->outputs =
        realloc(data->outputs, (data->numOutputs + 1) * sizeof(output_t *));
    if (data->outputs == NULL)
    {
        LL_DEBUG("Memory error in %s", __func__);
        return NULL;
    }

    LL_DEBUG("Allocating output: %s", (char*)type);

    tmpOutput = output_alloc();
    if (tmpOutput == NULL)
    {
        LL_DEBUG("Memory error in %s", __func__);
        return NULL;
    }

    if (parse_str_cmp("c", type))
    {
        tmpOutput->format = OUTPUT_FORMAT_C;
        tmpOutput->includeFileName = strdup("gfx.h");
    }
    else if (parse_str_cmp("asm", type))
    {
        tmpOutput->format = OUTPUT_FORMAT_ASM;
        tmpOutput->includeFileName = strdup("gfx.inc");
    }
    else if (parse_str_cmp("ice", type))
    {
        tmpOutput->format = OUTPUT_FORMAT_ICE;
        tmpOutput->includeFileName = strdup("ice.txt");
    }
    else if (parse_str_cmp("appvar", type))
    {
        tmpOutput->format = OUTPUT_FORMAT_APPVAR;
    }
    else if (parse_str_cmp("bin", type))
    {
        tmpOutput->format = OUTPUT_FORMAT_BIN;
        tmpOutput->includeFileName = strdup("gfx.txt");
    }
    else
    {
        LL_ERROR("Unknown output type.");
        return NULL;
    }

    data->outputs[data->numOutputs] = tmpOutput;
    data->numOutputs++;

    return tmpOutput;
}

/*
 * Allocate any builtings that exist when converting.
 */
static int parser_init(yaml_file_t *data)
{
    data->curPalette = NULL;
    data->curConvert = NULL;
    data->curOutput = NULL;
    data->palettes = NULL;
    data->converts = NULL;
    data->outputs = NULL;
    data->numPalettes = 0;
    data->numConverts = 0;
    data->numOutputs = 0;

    if (parser_alloc_palette(data, "xlibc") == NULL)
    {
        return 1;
    }
    if (parser_alloc_palette(data, "rgb332") == NULL)
    {
        return 1;
    }

    return 0;
}

/*
 * Parses a fixed palette color entry.
 */
static int parse_palette_entry(palette_entry_t *entry, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_pair_t *pair;

    memset(entry, 0, sizeof(palette_entry_t));

    entry->color.rgb.a = 255;
    entry->exact = false;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key = (char*)keyn->data.scalar.value;
        char *value = (char*)valuen->data.scalar.value;
        int tmpint;

        if (keyn != NULL)
        {
            tmpint = strtol(value, NULL, 0);
            if (tmpint > 255 || tmpint < 0)
            {
                LL_ERROR("Invalid fixed color option.");
                return 1;
            }

            if (parse_str_cmp("i", key) || parse_str_cmp("index", key))
            {
                entry->index = tmpint;
            }
            else if (parse_str_cmp("r", key) || parse_str_cmp("red", key))
            {
                entry->color.rgb.r = tmpint;
            }
            else if (parse_str_cmp("g", key) || parse_str_cmp("green", key))
            {
                entry->color.rgb.g = tmpint;
            }
            else if (parse_str_cmp("b", key) || parse_str_cmp("blue", key))
            {
                entry->color.rgb.b = tmpint;
            }
            else if (parse_str_cmp("exact", key))
            {
                entry->exact = parse_str_bool(value);
            }
            else
            {
                LL_ERROR("Unknown fixed color option.");
                parser_show_mark_error(keyn->start_mark);
                return 1;
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
    }

    LL_DEBUG("Adding fixed color: i: %d r: %d g: %d b: %d exact: %s",
        entry->index,
        entry->color.rgb.r,
        entry->color.rgb.g,
        entry->color.rgb.b,
        entry->exact ? "true" : "false");

    entry->origcolor = entry->color;

    return 0;
}

/*
 * Parses the fixed entry list.
 */
static int parse_palette_fixed_entry(palette_t *palette, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_pair_t *pair;
    palette_entry_t entry;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key = (char*)keyn->data.scalar.value;

        if (keyn != NULL)
        {
            if (parse_str_cmp("color", key))
            {
                if (parse_palette_entry(&entry, doc, valuen))
                {
                    return 1;
                }
                palette->fixedEntries[palette->numFixedEntries] = entry;
                palette->numFixedEntries++;
            }
            else
            {
                LL_ERROR("Unknown fixed entry option.");
                parser_show_mark_error(keyn->start_mark);
                return 1;
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
    }

    return 0;
}

/*
 * Gets each item in the fixed entry list for parsing.
 */
static int parse_palette_fixed_entries(palette_t *palette, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            int ret = parse_palette_fixed_entry(palette, doc, node);
            if (ret != 0)
            {
                return ret;
            }
        }
    }

    return 0;
}

/*
 * Adds images to the palette from paths.
 */
static int parse_palette_images(palette_t *palette, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item;

    switch (root->type)
    {
        case YAML_SEQUENCE_NODE:
            item = root->data.sequence.items.start;
            for (; item < root->data.sequence.items.top; ++item)
            {
                yaml_node_t *node = yaml_document_get_node(doc, *item);
                if (node != NULL)
                {
                    char *path = (char*)node->data.scalar.value;
                    if (pallete_add_path(palette, strings_trim(path)))
                    {
                        parser_show_mark_error(node->start_mark);
                        return 1;
                    }
                }
            }
            break;

        case YAML_SCALAR_NODE:
            if (parse_str_cmp("automatic", root->data.scalar.value))
            {
                LL_DEBUG("Using automatic palette generation.");
                palette->automatic = true;
            }
            else
            {
                LL_ERROR("Unknown palette images option.");
                parser_show_mark_error(root->start_mark);
                return 1;
            }
            break;

        default:
            LL_ERROR("Unknown palette images.");
            return 1;
    }

    return 0;
}

/*
 * Parses a palette.
 */
static int parse_palette(yaml_file_t *data, yaml_document_t *doc, yaml_node_t *root)
{
    palette_t *palette = NULL;
    yaml_node_pair_t *pair;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key = (char*)keyn->data.scalar.value;
        char *value = (char*)valuen->data.scalar.value;

        if (keyn != NULL)
        {
            if (parse_str_cmp("name", key))
            {
                palette = parser_alloc_palette(data, value);
            }
            else
            {
                int tmpi;

                if (palette == NULL)
                {
                    return 1;
                }
                if (parse_str_cmp("max-entries", key))
                {
                    tmpi = strtol(value, NULL, 0);
                    if (tmpi > 255 || tmpi < 2)
                    {
                        LL_ERROR("Invalid maxium entries.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                    palette->maxEntries = tmpi;
                }
                else if (parse_str_cmp("speed", key))
                {
                    tmpi = strtol(value, NULL, 0);
                    if (tmpi > 10 || tmpi < 1)
                    {
                        LL_ERROR("Invalid quantization speed.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                    palette->quantizeSpeed = tmpi;
                }
                else if (parse_str_cmp("fixed-entries", key))
                {
                    if (parse_palette_fixed_entries(palette, doc, valuen))
                    {
                        return 1;
                    }
                }
                else if (parse_str_cmp("images", key))
                {
                    if (parse_palette_images(palette, doc, valuen))
                    {
                        return 1;
                    }
                }
                else
                {
                    LL_ERROR("Unknown palette option: %s", key);
                    parser_show_mark_error(keyn->start_mark);
                    return 1;
                }
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
    }

    return 0;
}

/*
 * Gets indicies used for ommision in the convert.
 */
static int parse_convert_omits(convert_t *convert, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            int index = strtol((char*)node->data.scalar.value, NULL, 0);
            convert->omitIndices[convert->numOmitIndices] = index;
            convert->numOmitIndices++;
        }
    }

    return 0;
}

/*
 * Gets images used by the convert.
 */
static int parse_convert_images(convert_t *convert, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item;

    switch (root->type)
    {
        case YAML_SEQUENCE_NODE:
            item = root->data.sequence.items.start;
            for (; item < root->data.sequence.items.top; ++item)
            {
                yaml_node_t *node = yaml_document_get_node(doc, *item);
                if (node != NULL)
                {
                    char *path = (char*)node->data.scalar.value;
                    if (convert_add_image_path(convert, strings_trim(path)))
                    {
                        parser_show_mark_error(node->start_mark);
                        return 1;
                    }
                }
            }
            break;

        case YAML_SCALAR_NODE:
            if (parse_str_cmp("automatic", root->data.scalar.value))
            {
                LL_DEBUG("Using automatic convert generation.");
                if (convert_add_image_path(convert, "*"))
                {
                    parser_show_mark_error(root->start_mark);
                    return 1;
                }
            }
            else
            {
                LL_ERROR("Unknown convert images option.");
                parser_show_mark_error(root->start_mark);
                return 1;
            }
            break;

        default:
            LL_ERROR("Unknown convert images.");
            parser_show_mark_error(root->start_mark);
            return 1;
    }

    return 0;
}

/*
 * Gets tileset images used by the convert.
 */
static int parse_convert_tilesets_images(convert_t *convert, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item;

    switch (root->type)
    {
        case YAML_SEQUENCE_NODE:
            item = root->data.sequence.items.start;
            for (; item < root->data.sequence.items.top; ++item)
            {
                yaml_node_t *node = yaml_document_get_node(doc, *item);
                if (node != NULL)
                {
                    char *path = (char*)node->data.scalar.value;
                    if (convert_add_tileset_path(convert, strings_trim(path)))
                    {
                        parser_show_mark_error(node->start_mark);
                        return 1;
                    }
                }
            }
            break;

        case YAML_SCALAR_NODE:
            if (parse_str_cmp("automatic", root->data.scalar.value))
            {
                LL_DEBUG("Using automatic tilesets generation.");
                if (convert_add_tileset_path(convert, "*"))
                {
                    parser_show_mark_error(root->start_mark);
                    return 1;
                }
            }
            else
            {
                LL_ERROR("Unknown tilesets images option.");
                parser_show_mark_error(root->start_mark);
                return 1;
            }
            break;

        default:
            LL_ERROR("Unknown tilesets images.");
            parser_show_mark_error(root->start_mark);
            return 1;
    }

    return 0;
}

/*
 * Parses a convert tileset.
 */
static int parse_convert_tilesets(convert_t *convert, yaml_document_t *doc, yaml_node_t *root)
{
    tileset_group_t *group = NULL;
    yaml_node_pair_t *pair;

    if (root->type != YAML_MAPPING_NODE)
    {
        LL_ERROR("Unknown tileset options.");
        parser_show_mark_error(root->start_mark);
        return 1;
    }

    group = convert_alloc_tileset_group(convert);
    if (group == NULL)
    {
        return 1;
    }

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key = (char*)keyn->data.scalar.value;
        char *value = (char*)valuen->data.scalar.value;

        if (keyn != NULL)
        {
            if (parse_str_cmp("tile-width", key))
            {
                int tmpi = strtol(value, NULL, 0);
                if (tmpi > 255 || tmpi < 0)
                {
                    LL_ERROR("Invalid tileset tile width.");
                    parser_show_mark_error(keyn->start_mark);
                    return 1;
                }
                group->tileWidth = tmpi;
            }
            else if (parse_str_cmp("tile-height", key))
            {
                int tmpi = strtol(value, NULL, 0);
                if (tmpi > 255 || tmpi < 0)
                {
                    LL_ERROR("Invalid tileset tile height.");
                    parser_show_mark_error(keyn->start_mark);
                    return 1;
                }
                group->tileHeight = tmpi;
            }
            else if (parse_str_cmp("pointer-table", key))
            {
                group->pTable = parse_str_bool(value);
            }
            else if (parse_str_cmp("images", key))
            {
                if (parse_convert_tilesets_images(convert, doc, valuen))
                {
                    return 1;
                }
            }
            else
            {
                LL_ERROR("Unknown tilesets option: %s", key);
                parser_show_mark_error(keyn->start_mark);
                return 1;
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
    }


    return 0;
}

/*
 * Parses a convert.
 */
static int parse_convert(yaml_file_t *data, yaml_document_t *doc, yaml_node_t *root)
{
    convert_t *convert = NULL;
    yaml_node_pair_t *pair;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key = (char*)keyn->data.scalar.value;
        char *value = (char*)valuen->data.scalar.value;

        if (keyn != NULL)
        {
            if (parse_str_cmp("name", key))
            {
                convert = parser_alloc_convert(data, value);
            }
            else
            {
                int tmpf;
                int tmpi;

                if (convert == NULL)
                {
                    return 1;
                }
                if (parse_str_cmp("palette", key))
                {
                    if (convert->paletteName != NULL)
                    {
                        free(convert->paletteName);
                    }
                    convert->paletteName = strdup(value);
                }
                else if (parse_str_cmp("style", key))
                {
                    if (parse_str_cmp("rlet", value))
                    {
                        convert->style = CONVERT_STYLE_RLET;
                    }
                    else
                    {
                        LL_ERROR("Invalid convert style.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                }
                else if (parse_str_cmp("compress", key))
                {
                    if (parse_str_cmp("zx7", value))
                    {
                        convert->style = COMPRESS_ZX7;
                    }
                    else
                    {
                        LL_ERROR("Invalid compression mode.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                }
                else if (parse_str_cmp("dither", key))
                {
                    tmpf = strtof(value, NULL);
                    if (tmpf > 1 || tmpf < 0)
                    {
                        LL_ERROR("Invalid dither value.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                    convert->dither = tmpf;
                }
                else if (parse_str_cmp("rotate", key))
                {
                    tmpi = strtof(value, NULL);
                    if (tmpi != 0 && tmpi != 90 && tmpi != 180 && tmpi != 270)
                    {
                        LL_ERROR("Invalid rotate parameter, must be 0, 90, 180, or 270.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                    convert->rotate = tmpi;
                }
                else if (parse_str_cmp("speed", key))
                {
                    tmpi = strtol(value, NULL, 0);
                    if (tmpi > 10 || tmpi < 1)
                    {
                        LL_ERROR("Invalid quantization speed.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                    convert->quantizeSpeed = tmpi;
                }
                else if (parse_str_cmp("transparent-color-index", key))
                {
                    tmpi = strtol(value, NULL, 0);
                    if (tmpi >= PALETTE_MAX_ENTRIES || tmpi < 0)
                    {
                        LL_ERROR("Invalid transparent color index.");
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                    convert->transparentIndex = tmpi;
                }
                else if (parse_str_cmp("bpp", key))
                {
                    switch (strtol(value, NULL, 0))
                    {
                        case 8: convert->bpp = BPP_8; break;
                        case 4: convert->bpp = BPP_4; break;
                        case 2: convert->bpp = BPP_2; break;
                        case 1: convert->bpp = BPP_1; break;
                        default:
                            LL_ERROR("Invalid bpp option.");
                            parser_show_mark_error(keyn->start_mark);
                            return 1;
                    }
                }
                else if (parse_str_cmp("flip-x", key))
                {
                    convert->flipx = parse_str_bool(value);
                }
                else if (parse_str_cmp("flip-y", key))
                {
                    convert->flipy = parse_str_bool(value);
                }
                else if (parse_str_cmp("width-and-height", key))
                {
                    convert->widthAndHeight = parse_str_bool(value);
                }
                else if (parse_str_cmp("omit-indices", key))
                {
                    parse_convert_omits(convert, doc, valuen);
                }
                else if (parse_str_cmp("images", key))
                {
                    if (parse_convert_images(convert, doc, valuen))
                    {
                        return 1;
                    }
                }
                else if (parse_str_cmp("tilesets", key))
                {
                    if (parse_convert_tilesets(convert, doc, valuen))
                    {
                        return 1;
                    }
                }
                else
                {
                    LL_ERROR("Unknown convert option: %s", key);
                    parser_show_mark_error(keyn->start_mark);
                    return 1;
                }
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
    }

    return 0;
}

/*
 * Gets palettes to output.
 */
static int parse_output_palettes(output_t *output, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            char *name = (char*)node->data.scalar.value;
            if (output_add_palette(output, strings_trim(name)))
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Gets converts to output.
 */
static int parse_output_converts(output_t *output, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            char *name = (char*)node->data.scalar.value;
            if (output_add_convert(output, strings_trim(name)))
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Parses an output.
 */
static int parse_output(yaml_file_t *data, yaml_document_t *doc, yaml_node_t *root)
{
    output_t *output = NULL;
    yaml_node_pair_t *pair;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key = (char*)keyn->data.scalar.value;
        char *value = (char*)valuen->data.scalar.value;

        if (keyn != NULL)
        {
            if (parse_str_cmp("type", key))
            {
                output = parser_alloc_output(data, value);
                if (output == NULL)
                {
                    return 1;
                }
            }
            else
            {
                if (parse_str_cmp("include-file", key))
                {
                    if (output->includeFileName != NULL)
                    {
                        free(output->includeFileName);
                    }
                    output->includeFileName = strdup(value);
                }
                else if (parse_str_cmp("directory", key))
                {
                    if (output->directory != NULL)
                    {
                        free(output->directory);
                        output->directory = NULL;
                    }
                    char *tmp = strdup(value);
                    if (tmp == NULL)
                    {
                        LL_DEBUG("Memory error in %s", __func__);
                        return 1;
                    }
                    if (*tmp && tmp[strlen(tmp) - 1] != '/')
                    {
                        output->directory = strdupcat(tmp, "/");
                        free(tmp);
                    }
                    else
                    {
                        output->directory = tmp;
                    }
                }
                else if (parse_str_cmp("palettes", key))
                {
                    if (parse_output_palettes(output, doc, valuen))
                    {
                        return 1;
                    }
                }
                else if (parse_str_cmp("converts", key))
                {
                    if (parse_output_converts(output, doc, valuen))
                    {
                        return 1;
                    }
                }
                else
                {
                    if (output->format != OUTPUT_FORMAT_APPVAR)
                    {
                        LL_ERROR("Unknown output option: %s", key);
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }

                    if (parse_str_cmp("name", key))
                    {
                        if (output->appvar.name != NULL)
                        {
                            free(output->appvar.name);
                        }
                        output->appvar.name = strdup(value);
                    }
                    else if (parse_str_cmp("archived", key))
                    {
                        output->appvar.archived = parse_str_bool(value);
                    }
                    else if (parse_str_cmp("source-init", key))
                    {
                        output->appvar.init = parse_str_bool(value);
                    }
                    else if (parse_str_cmp("source-format", key))
                    {
                        if (parse_str_cmp("c", value))
                        {
                            output->appvar.source = APPVAR_SOURCE_C;
                        }
                        else if (parse_str_cmp("ice", value))
                        {
                            output->appvar.source = APPVAR_SOURCE_ICE;
                        }
                        else if (parse_str_cmp("raw", value))
                        {
                            output->appvar.source = APPVAR_SOURCE_ICE;
                        }
                        else
                        {
                            LL_ERROR("Unknown AppVar source format.");
                            parser_show_mark_error(keyn->start_mark);
                            return 1;
                        }
                    }
                    else if (parse_str_cmp("compress", key))
                    {
                        if (parse_str_cmp("zx7", value))
                        {
                            output->appvar.compress = COMPRESS_ZX7;
                        }
                        else
                        {
                            LL_ERROR("Invalid compression mode.");
                            parser_show_mark_error(keyn->start_mark);
                            return 1;
                        }
                    }
                    else if (parse_str_cmp("header-string", key))
                    {
                        output->appvar.header = strdup(value);
                        output->appvar.header_size = strlen(output->appvar.header);
                    }
                    else
                    {
                        LL_ERROR("Unknown appvar output option: %s", key);
                        parser_show_mark_error(keyn->start_mark);
                        return 1;
                    }
                }
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
    }

    return 0;
}

/*
 * Parses all palettes.
 */
static int parse_palettes(yaml_file_t *data, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            if (parse_palette(data, doc, node))
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Parses all converts.
 */
static int parse_converts(yaml_file_t *data, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            if (parse_convert(data, doc, node))
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Parses all outputs.
 */
static int parse_outputs(yaml_file_t *data, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            if (parse_output(data, doc, node))
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Parses the whole YAML file.
 */
static int parse_yaml(yaml_file_t *data, yaml_document_t *doc)
{
    yaml_node_t *root = yaml_document_get_root_node(doc);
    yaml_node_pair_t *pair;
    int ret = 0;

    if (root == NULL || root->type != YAML_MAPPING_NODE)
    {
        LL_ERROR("No root node detected.");
        parser_show_mark_error(root->start_mark);
        return 1;
    }

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *key = yaml_document_get_node(doc, pair->key);
        yaml_node_t *value = yaml_document_get_node(doc, pair->value);
        if (key != NULL)
        {
            if (parse_str_cmp("palettes", key->data.scalar.value))
            {
                ret = parse_palettes(data, doc, value);
            }
            if (parse_str_cmp("converts", key->data.scalar.value))
            {
                ret = parse_converts(data, doc, value);
            }
            if (parse_str_cmp("outputs", key->data.scalar.value))
            {
                ret = parse_outputs(data, doc, value);
            }
        }
        else
        {
            LL_ERROR("No node.");
            return 1;
        }
        if (ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

/*
 * Parses a YAML file and stores the results to a structure.
 */
int yaml_parse_file(yaml_file_t *data)
{
    yaml_parser_t yaml;
    yaml_document_t document;
    FILE *fdi;
    int ret = 0;

    if (data == NULL)
    {
        LL_DEBUG("Invalid param in %s", __func__);
        return 1;
    }

    if (parser_init(data))
    {
        return 1;
    }

    LL_INFO("Reading file \'%s\'", data->name);

    fdi = fopen(data->name, "rb");
    if (fdi == NULL)
    {
        LL_ERROR("Could not open file: %s", strerror(errno));
        LL_ERROR("Use --help option if needed.");
        return 1;
    }

    if (!yaml_parser_initialize(&yaml))
    {
        LL_ERROR("Could not initialize YAML parser.");
        return 1;
    }

    yaml_parser_set_input_file(&yaml, fdi);
    if (!yaml_parser_load(&yaml, &document))
    {
        parser_show_error(yaml.problem, yaml.mark);
        yaml_parser_delete(&yaml);
        fclose(fdi);
        return 1;
    }

    ret = parse_yaml(data, &document);

    yaml_document_delete(&document);
    yaml_parser_delete(&yaml);

    fclose(fdi);

    return ret;
}

/*
 * Frees structures in yaml.
 */
void yaml_release_file(yaml_file_t *yamlfile)
{
    int i;

    for (i = 0; i < yamlfile->numOutputs; ++i)
    {
        output_free(yamlfile->outputs[i]);
        free(yamlfile->outputs[i]);
        yamlfile->outputs[i] = NULL;
    }

    free(yamlfile->outputs);
    yamlfile->outputs = NULL;

    for (i = 0; i < yamlfile->numConverts; ++i)
    {
        convert_free(yamlfile->converts[i]);
        free(yamlfile->converts[i]);
        yamlfile->converts[i] = NULL;
    }

    free(yamlfile->converts);
    yamlfile->converts = NULL;

    for (i = 0; i < yamlfile->numPalettes; ++i)
    {
        palette_free(yamlfile->palettes[i]);
        free(yamlfile->palettes[i]);
        yamlfile->palettes[i] = NULL;
    }

    free(yamlfile->palettes);
    yamlfile->palettes = NULL;

    free(yamlfile->name);
    yamlfile->name = NULL;
}
