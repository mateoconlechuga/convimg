/*
 * Copyright 2017-2024 Matt "MateoConLechuga" Waltz
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

#include "parser.h"
#include "strings.h"
#include "memory.h"
#include "log.h"

#include "deps/libyaml/include/yaml.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

static bool parse_str_cmp(const char *str, void *src)
{
    if (str == NULL || src == NULL)
    {
        return false;
    }

    if (strncmp(str, src, strlen(str)) == 0)
    {
        return strlen(str) == strlen(src);
    }

    return false;
}

static bool parse_str_bool(void *src)
{
    return parse_str_cmp("true", src);
}

static void parser_show_mark_error(yaml_mark_t mark)
{
    LOG_ERROR("Problem is probably around line %" PRIuPTR ".\n", mark.line + 1);
}

static void parse_show_error(const char *problem, yaml_mark_t mark)
{
    LOG_ERROR("Problem description: %s\n", problem);
    parser_show_mark_error(mark);
}

static struct palette *parser_alloc_palette(struct yaml *yaml, void *name)
{
    struct palette *palette;

    LOG_DEBUG("Allocating palette: %s\n", (char*)name);

    yaml->palettes = memory_realloc_array(yaml->palettes, yaml->nr_palettes + 1, sizeof(struct palette *));
    if (yaml->palettes == NULL)
    {
        return NULL;
    }

    palette = palette_alloc();
    if (palette == NULL)
    {
        return NULL;
    }

    yaml->palettes[yaml->nr_palettes] = palette;
    yaml->nr_palettes++;

    palette->name = strings_dup(name);
    if (palette->name == NULL)
    {
        return NULL;
    }

    for (uint32_t i = 0; i < PALETTE_MAX_ENTRIES; ++i)
    {
        struct palette_entry *entry = &palette->entries[i];

        entry->valid = false;
        entry->exact = false;
        entry->target = 0;
        entry->color.r = 0;
        entry->color.g = 0;
        entry->color.b = 0;
        entry->orig_color.r = 0;
        entry->orig_color.g = 0;
        entry->orig_color.b = 0;
    }

    return palette;
}

static struct convert *parser_alloc_convert(struct yaml *yaml, void *name)
{
    struct convert *convert;

    LOG_DEBUG("Allocating convert: %s\n", (char*)name);

    yaml->converts = memory_realloc_array(yaml->converts, yaml->nr_converts + 1, sizeof(struct convert *));
    if (yaml->converts == NULL)
    {
        return NULL;
    }

    convert = convert_alloc();
    if (convert == NULL)
    {
        return NULL;
    }

    yaml->converts[yaml->nr_converts] = convert;
    yaml->nr_converts++;

    convert->name = strings_dup(name);
    if (convert->name == NULL)
    {
        return NULL;
    }

    return convert;
}

static struct output *parser_alloc_output(struct yaml *yaml, void *type)
{
    const char *include_file;
    struct output *output;

    LOG_DEBUG("Allocating output: %s\n", (char*)type);

    yaml->outputs = memory_realloc_array(yaml->outputs, yaml->nr_outputs + 1, sizeof(struct output *));
    if (yaml->outputs == NULL)
    {
        return NULL;
    }

    output = output_alloc();
    if (output == NULL)
    {
        return NULL;
    }

    yaml->outputs[yaml->nr_outputs] = output;
    yaml->nr_outputs++;

    if (parse_str_cmp("c", type))
    {
        output->format = OUTPUT_FORMAT_C;
        include_file = "gfx.h";
    }
    else if (parse_str_cmp("asm", type))
    {
        output->format = OUTPUT_FORMAT_ASM;
        include_file = "gfx.inc";
    }
    else if (parse_str_cmp("ice", type))
    {
        output->format = OUTPUT_FORMAT_BASIC;
        include_file = "ice.txt";
    }
    else if (parse_str_cmp("basic", type))
    {
        output->format = OUTPUT_FORMAT_BASIC;
        include_file = "basic.txt";
    }
    else if (parse_str_cmp("appvar", type))
    {
        output->format = OUTPUT_FORMAT_APPVAR;
        include_file = "gfx.h";
    }
    else if (parse_str_cmp("bin", type))
    {
        output->format = OUTPUT_FORMAT_BIN;
        include_file = "gfx.txt";
    }
    else
    {
        LOG_ERROR("Unknown output type.\n");
        return NULL;
    }

    output->include_file = strings_dup(include_file);
    if (output->include_file == NULL)
    {
        return NULL;
    }

    return output;
}

static int parser_init(struct yaml *yaml)
{
    yaml->palettes = NULL;
    yaml->converts = NULL;
    yaml->outputs = NULL;
    yaml->cur_palette = NULL;
    yaml->cur_convert = NULL;
    yaml->cur_output = NULL;
    yaml->nr_palettes = 0;
    yaml->nr_converts = 0;
    yaml->nr_outputs = 0;

    if (parser_alloc_palette(yaml, "xlibc") == NULL)
    {
        return -1;
    }

    if (parser_alloc_palette(yaml, "rgb332") == NULL)
    {
        return -1;
    }

    return 0;
}

static int parse_palette_entry(struct palette_entry *entry, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_pair_t *pair;

    memset(entry, 0, sizeof(struct palette_entry));

    entry->exact = false;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        char *value;
        char *key;

        if (keyn == NULL || valuen == NULL)
        {
            continue;
        }

        key = (char*)keyn->data.scalar.value;
        value = (char*)valuen->data.scalar.value;

        if (value == NULL || value[0] == '\0')
        {
            LOG_ERROR("Invalid fixed entry color format -- are you missing indentation?\n");
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }

        if (parse_str_cmp("i", key) || parse_str_cmp("index", key))
        {
            int tmpi = strtol(value, NULL, 0);
            if (tmpi > 255 || tmpi < 0)
            {
                LOG_ERROR("Invalid fixed color index.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            entry->index = tmpi;
        }
        else if (parse_str_cmp("r", key) || parse_str_cmp("red", key))
        {
            int tmpi = strtol(value, NULL, 0);
            if (tmpi > 255 || tmpi < 0)
            {
                LOG_ERROR("Invalid fixed color red value.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            entry->color.r = tmpi;
        }
        else if (parse_str_cmp("g", key) || parse_str_cmp("green", key))
        {
            int tmpi = strtol(value, NULL, 0);
            if (tmpi > 255 || tmpi < 0)
            {
                LOG_ERROR("Invalid fixed color green value.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            entry->color.g = tmpi;
        }
        else if (parse_str_cmp("b", key) || parse_str_cmp("blue", key))
        {
            int tmpi = strtol(value, NULL, 0);
            if (tmpi > 255 || tmpi < 0)
            {
                LOG_ERROR("Invalid fixed color blue value.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            entry->color.b = tmpi;
        }
        else if (parse_str_cmp("hex", key))
        {
            if (!strings_hex_color(value, &entry->color))
            {
                LOG_ERROR("Invalid fixed color hex value.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
        }
        else if (parse_str_cmp("exact", key))
        {
            entry->exact = parse_str_bool(value);
        }
        else
        {
            LOG_ERROR("Unknown fixed color option.\n");
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }
    }

    LOG_DEBUG("Adding fixed color: i: %u r: %u g: %u b: %u exact: %s\n",
        entry->index,
        entry->color.r,
        entry->color.g,
        entry->color.b,
        entry->exact ? "true" : "false");

    entry->orig_color = entry->color;

    return 0;
}

static int parse_palette_image(struct palette *palette, const char *path)
{
    struct image image;
    uint32_t i;

    image_init(&image, path);

    if (image_load(&image))
    {
        goto fail;
    }

    if (image.height != 1 || image.width > 256)
    {
        LOG_ERROR("Invalid palette image format.\n");
        goto fail;
    }

    for (i = 0; i < image.width; ++i)
    {
        struct palette_entry entry;
        uint8_t alpha;
        uint32_t o = i * sizeof(uint32_t);
        uint32_t j;

        memset(&entry, 0, sizeof entry);

        entry.exact = false;

        entry.index = i;
        entry.color.r = image.data[o + 0];
        entry.color.g = image.data[o + 1];
        entry.color.b = image.data[o + 2];
        alpha = image.data[o + 3];

        if (alpha != 255)
        {
            LOG_ERROR("Palette image \'%s\' has alpha transparency.\n",
                palette->name);
            goto fail;
        }

        if (palette->nr_fixed_entries > PALETTE_MAX_ENTRIES)
        {
            LOG_ERROR("Too many fixed colors for palette \'%s\'.\n",
                palette->name);
            goto fail;
        }

        for (j = 0; j < palette->nr_fixed_entries; ++j)
        {
            if (palette->fixed_entries[j].index == i)
            {
                LOG_WARNING("Overriding palette index %u with (r: %d, g: %d, b: %d).\n",
                    entry.index,
                    entry.color.r,
                    entry.color.g,
                    entry.color.b);
            }
        }

        entry.orig_color = entry.color;

        LOG_DEBUG("Adding fixed color: i: %u r: %d g: %d b: %d exact: %s\n",
            entry.index,
            entry.color.r,
            entry.color.g,
            entry.color.b,
            entry.exact ? "true" : "false");

        palette->fixed_entries[palette->nr_fixed_entries] = entry;
        palette->nr_fixed_entries++;
    }

    image_free(&image);

    return 0;

fail:
    image_free(&image);

    return -1;
}

static int parse_palette_fixed_entry(struct palette *palette, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_pair_t *pair;
    struct palette_entry entry;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key;

        if (keyn == NULL || valuen == NULL)
        {
            continue;
        }

        key = (char*)keyn->data.scalar.value;

        if (parse_str_cmp("color", key))
        {
            size_t j;

            if (palette->nr_fixed_entries > PALETTE_MAX_ENTRIES)
            {
                LOG_ERROR("Too many fixed colors for palette \'%s\'.\n", palette->name);
                return -1;
            }
            if (parse_palette_entry(&entry, doc, valuen))
            {
                return -1;
            }

            for (j = 0; j < palette->nr_fixed_entries; ++j)
            {
                if (palette->fixed_entries[j].index == entry.index)
                {
                    LOG_WARNING("Overriding palette index %u with (r: %d, g: %d, b: %d).\n",
                        entry.index,
                        entry.color.r,
                        entry.color.g,
                        entry.color.b);
                }
            }
            palette->fixed_entries[palette->nr_fixed_entries] = entry;
            palette->nr_fixed_entries++;
        }
        else if (parse_str_cmp("image", key))
        {
            if (parse_palette_image(palette, (const char*)valuen->data.scalar.value))
            {
                return -1;
            }
        }
        else
        {
            LOG_ERROR("Unknown fixed entry option.\n");
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }
    }

    return 0;
}

static int parse_palette_fixed_entries(struct palette *palette, yaml_document_t *doc, yaml_node_t *root)
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

static int parse_palette_images(struct palette *palette, yaml_document_t *doc, yaml_node_t *root)
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
                        return -1;
                    }
                }
            }
            break;

        case YAML_SCALAR_NODE:
            if (parse_str_cmp("automatic", root->data.scalar.value))
            {
                LOG_DEBUG("Using automatic palette generation.\n");
                palette->automatic = true;
            }
            else
            {
                LOG_ERROR("Unknown palette images option.\n");
                parser_show_mark_error(root->start_mark);
                return -1;
            }
            break;

        default:
            LOG_ERROR("Unknown palette images.\n");
            return -1;
    }

    return 0;
}

static int parse_palette(struct yaml *data, yaml_document_t *doc, yaml_node_t *root)
{
    struct palette *palette = NULL;
    yaml_node_pair_t *pair;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key;
        char *value;

        if (keyn == NULL || valuen == NULL)
        {
            continue;
        }

        key = (char*)keyn->data.scalar.value;
        value = (char*)valuen->data.scalar.value;

        if (palette == NULL && parse_str_cmp("name", key))
        {
            palette = parser_alloc_palette(data, value);
            if (palette == NULL)
            {
                return -1;
            }
        }
        else
        {
            int tmpi;

            if (palette == NULL)
            {
                LOG_ERROR("Invalid palette formatting. "
                    "Be sure to use the \'name\' parameter for the palettes list.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }

            if (parse_str_cmp("max-entries", key))
            {
                tmpi = strtol(value, NULL, 0);
                if (tmpi > 256 || tmpi < 2)
                {
                    LOG_ERROR("Invalid palette \'max-entries\' parameter.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
                palette->max_entries = tmpi;
            }
            else if (parse_str_cmp("speed", key))
            {
                tmpi = strtol(value, NULL, 0);
                if (tmpi > 10 || tmpi < 1)
                {
                    LOG_ERROR("Invalid palette \'speed\' parameter.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
                palette->quantize_speed = tmpi;
            }
            else if (parse_str_cmp("color-format", key))
            {
                if (parse_str_cmp("grgb1555", value))
                {
                    palette->color_fmt = COLOR_1555_GRGB;
                }
                else if (parse_str_cmp("rgb565", value))
                {
                    palette->color_fmt = COLOR_565_RGB;
                }
                else if (parse_str_cmp("bgr565", value))
                {
                    palette->color_fmt = COLOR_565_BGR;
                }
                else
                {
                    LOG_ERROR("Invalid \'color-format\' option.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
            }
            else if (parse_str_cmp("quality", key))
            {
                tmpi = strtol(value, NULL, 0);
                if (tmpi > 10 || tmpi < 1)
                {
                    LOG_ERROR("Invalid palette \'quality\' parameter.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
                palette->quantize_speed = 11 - tmpi;
            }
            else if (parse_str_cmp("fixed-entries", key))
            {
                if (parse_palette_fixed_entries(palette, doc, valuen))
                {
                    return -1;
                }
            }
            else if (parse_str_cmp("images", key))
            {
                if (parse_palette_images(palette, doc, valuen))
                {
                    return -1;
                }
            }
            else
            {
                LOG_ERROR("Unknown palette option: %s\n", key);
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
        }
    }

    return 0;
}

static int parse_convert_omits(struct convert *convert, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            long index = strtol((char *)node->data.scalar.value, NULL, 0);

            if (index > 255 || index < 0)
            {
                LOG_ERROR("Invalid omit index value %ld\n", index);
                parser_show_mark_error(node->start_mark);
                return -1;
            }

            convert->omit_indices[convert->nr_omit_indices] = index;
            convert->nr_omit_indices++;
        }
    }

    return 0;
}

static int parse_convert_images(struct convert *convert, yaml_document_t *doc, yaml_node_t *root)
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
                        return -1;
                    }
                }
            }
            break;

        case YAML_SCALAR_NODE:
            if (parse_str_cmp("automatic", root->data.scalar.value))
            {
                LOG_DEBUG("Using automatic convert generation.\n");
                if (convert_add_image_path(convert, "*"))
                {
                    parser_show_mark_error(root->start_mark);
                    return -1;
                }
            }
            else
            {
                LOG_ERROR("Unknown convert images option.\n");
                parser_show_mark_error(root->start_mark);
                return -1;
            }
            break;

        default:
            LOG_ERROR("Unknown convert images.\n");
            parser_show_mark_error(root->start_mark);
            return -1;
    }

    return 0;
}

static int parse_convert_tilesets_images(struct convert *convert, yaml_document_t *doc, yaml_node_t *root)
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
                        return -1;
                    }
                }
            }
            break;

        case YAML_SCALAR_NODE:
            if (parse_str_cmp("automatic", root->data.scalar.value))
            {
                LOG_DEBUG("Using automatic tilesets generation.\n");
                if (convert_add_tileset_path(convert, "*"))
                {
                    parser_show_mark_error(root->start_mark);
                    return -1;
                }
            }
            else
            {
                LOG_ERROR("Unknown tilesets images option.\n");
                parser_show_mark_error(root->start_mark);
                return -1;
            }
            break;

        default:
            LOG_ERROR("Unknown tilesets images.\n");
            parser_show_mark_error(root->start_mark);
            return -1;
    }

    return 0;
}

static int parse_convert_tilesets(struct convert *convert, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_pair_t *pair;

    if (root->type != YAML_MAPPING_NODE)
    {
        LOG_ERROR("Unknown tileset options.\n");
        parser_show_mark_error(root->start_mark);
        return -1;
    }

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key;
        char *value;

        if (keyn == NULL || valuen == NULL)
        {
            continue;
        }

        key = (char*)keyn->data.scalar.value;
        value = (char*)valuen->data.scalar.value;

        if (parse_str_cmp("tile-width", key))
        {
            int tmpi;
            if (convert->tile_width)
            {
                LOG_ERROR("Tileset tile-width can only be specified once per convert.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            tmpi = strtol(value, NULL, 0);
            if (tmpi < 1)
            {
                LOG_ERROR("Invalid tileset tile-width: %d\n", tmpi);
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->tile_width = tmpi;
        }
        else if (parse_str_cmp("tile-height", key))
        {
            int tmpi;
            if (convert->tile_height)
            {
                LOG_ERROR("Tileset tile-height can only be specified once per convert.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            tmpi = strtol(value, NULL, 0);
            if (tmpi < 1)
            {
                LOG_ERROR("Invalid tileset tile-height: %d\n", tmpi);
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->tile_height = tmpi;
        }
        else if (parse_str_cmp("pointer-table", key))
        {
            convert->p_table = parse_str_bool(value);
        }
        else if (parse_str_cmp("tile-rotate", key))
        {
            int tmpi = strtol(value, NULL, 10);
            if (tmpi != 0 && tmpi != 90 && tmpi != 180 && tmpi != 270)
            {
                LOG_ERROR("Invalid rotate parameter, must be 0, 90, 180, or 270.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->tile_rotate = tmpi;
        }
        else if (parse_str_cmp("tile-flip-x", key))
        {
            convert->tile_flip_x = parse_str_bool(value);
        }
        else if (parse_str_cmp("tile-flip-y", key))
        {
            convert->tile_flip_y = parse_str_bool(value);
        }
        else if (parse_str_cmp("images", key))
        {
            if (parse_convert_tilesets_images(convert, doc, valuen))
            {
                return -1;
            }
        }
        else
        {
            LOG_ERROR("Unknown tilesets option: \'%s\'\n", key);
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }
    }

    return 0;
}

static int parse_convert(struct yaml *data, yaml_document_t *doc, yaml_node_t *root)
{
    struct convert *convert = NULL;
    yaml_node_pair_t *pair;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        char *key;
        char *value;
        int tmpi;

        if (keyn == NULL || valuen == NULL)
        {
            continue;
        }

        key = (char*)keyn->data.scalar.value;
        value = (char*)valuen->data.scalar.value;

        if (convert == NULL && parse_str_cmp("name", key))
        {
            convert = parser_alloc_convert(data, value);
            if (convert == NULL)
            {
                return -1;
            }
            continue;
        }

        if (convert == NULL)
        {
            LOG_ERROR("Invalid convert formatting. "
                "Be sure to use the \'name\' parameter for the converts list.\n");
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }

        if (parse_str_cmp("palette", key))
        {
            if (convert->palette_name != NULL)
            {
                free(convert->palette_name);
            }
            convert->palette_name = strings_dup(value);
            if (convert->palette_name == NULL)
            {
                return -1;
            }
        }
        else if (parse_str_cmp("style", key))
        {
            if (parse_str_cmp("normal", value))
            {
                convert->style = CONVERT_STYLE_PALETTE;
            }
            else if (parse_str_cmp("palette", value))
            {
                convert->style = CONVERT_STYLE_PALETTE;
            }
            else if (parse_str_cmp("rlet", value))
            {
                convert->style = CONVERT_STYLE_RLET;
            }
            else if (parse_str_cmp("direct", value))
            {
                convert->style = CONVERT_STYLE_DIRECT;
            }
            else
            {
                LOG_ERROR("Invalid convert style.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
        }
        else if (parse_str_cmp("color-format", key))
        {
            if (parse_str_cmp("grgb1555", value))
            {
                convert->color_fmt = COLOR_1555_GRGB;
            }
            else if (parse_str_cmp("rgb565", value))
            {
                convert->color_fmt = COLOR_565_RGB;
            }
            else if (parse_str_cmp("bgr565", value))
            {
                convert->color_fmt = COLOR_565_BGR;
            }
            else if (parse_str_cmp("rgb888", value))
            {
                convert->color_fmt = COLOR_888_RGB;
            }
            else if (parse_str_cmp("bgr888", value))
            {
                convert->color_fmt = COLOR_888_BGR;
            }
            else
            {
                LOG_ERROR("Invalid \'color-format\' option.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
        }
        else if (parse_str_cmp("compress", key))
        {
            if (parse_str_cmp("zx7", value))
            {
                convert->compress = COMPRESS_ZX7;
            }
            else if (parse_str_cmp("zx0", value))
            {
                convert->compress = COMPRESS_ZX0;
            }
            else
            {
                LOG_ERROR("Invalid compression mode.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
        }
        else if (parse_str_cmp("dither", key))
        {
            float tmpf = strtof(value, NULL);
            if (tmpf > 1 || tmpf < 0)
            {
                LOG_ERROR("Invalid dither value.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->dither = tmpf;
        }
        else if (parse_str_cmp("rotate", key))
        {
            tmpi = strtol(value, NULL, 10);
            if (tmpi != 0 && tmpi != 90 && tmpi != 180 && tmpi != 270)
            {
                LOG_ERROR("Invalid rotate parameter, must be 0, 90, 180, or 270.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->rotate = tmpi;
        }
        else if (parse_str_cmp("speed", key))
        {
            tmpi = strtol(value, NULL, 10);
            if (tmpi > 10 || tmpi < 1)
            {
                LOG_ERROR("Invalid quantization speed.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->quantize_speed = tmpi;
        }
        else if (parse_str_cmp("quality", key))
        {
            tmpi = strtol(value, NULL, 10);
            if (tmpi > 10 || tmpi < 1)
            {
                LOG_ERROR("Invalid quantization quality.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->quantize_speed = 11 - tmpi;
        }
        else if (parse_str_cmp("transparent-index", key) ||
                 parse_str_cmp("transparent-color-index", key))
        {
            tmpi = strtol(value, NULL, 0);
            if (tmpi >= PALETTE_MAX_ENTRIES || tmpi < 0)
            {
                LOG_ERROR("Invalid transparent color index.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->transparent_index = tmpi;
        }
        else if (parse_str_cmp("palette-offset", key))
        {
            tmpi = strtol(value, NULL, 0);
            if (tmpi >= PALETTE_MAX_ENTRIES || tmpi < 0)
            {
                LOG_ERROR("Invalid palette offset.\n");
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
            convert->palette_offset = tmpi;
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
                    LOG_ERROR("Invalid bpp option.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
            }
        }
        else if (parse_str_cmp("flip-x", key))
        {
            convert->flip_x = parse_str_bool(value);
        }
        else if (parse_str_cmp("flip-y", key))
        {
            convert->flip_y = parse_str_bool(value);
        }
        else if (parse_str_cmp("width-and-height", key))
        {
            convert->add_width_height = parse_str_bool(value);
        }
        else if (parse_str_cmp("omit-indices", key))
        {
            if (parse_convert_omits(convert, doc, valuen))
            {
                return -1;
            }
        }
        else if (parse_str_cmp("images", key))
        {
            if (parse_convert_images(convert, doc, valuen))
            {
                return -1;
            }
        }
        else if (parse_str_cmp("tilesets", key))
        {
            if (parse_convert_tilesets(convert, doc, valuen))
            {
                return -1;
            }
        }
        else
        {
            LOG_ERROR("Unknown convert option: \'%s\'\n", key);
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }
    }

    return 0;
}

static int parse_output_palettes(struct output *output, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            char *name = (char*)node->data.scalar.value;
            if (output_add_palette_name(output, strings_trim(name)) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int parse_output_converts(struct output *output, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL)
        {
            char *name = (char*)node->data.scalar.value;
            if (output_add_convert_name(output, strings_trim(name)) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int parse_output(struct yaml *yaml, yaml_document_t *doc, yaml_node_t *root)
{
    struct output *output = NULL;
    yaml_node_pair_t *pair;

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);

        if (keyn == NULL || valuen == NULL)
        {
            LOG_ERROR("Invalid YAML outputs format.\n");
            if (keyn != NULL)
            {
                parser_show_mark_error(keyn->start_mark);
            }
            return -1;
        }

        char *key = (char *)keyn->data.scalar.value;
        char *value = (char *)valuen->data.scalar.value;
        int valuelen = (int)valuen->data.scalar.length;

        if (parse_str_cmp("type", key))
        {
            output = parser_alloc_output(yaml, value);
            if (output == NULL)
            {
                return -1;
            }
            continue;
        }

        if (output == NULL)
        {
            LOG_ERROR("Invalid output formatting. "
                "Be sure to use the \'type\' parameter for the outputs list.\n");
            parser_show_mark_error(keyn->start_mark);
            return -1;
        }

        if (parse_str_cmp("include-file", key))
        {
            if (output->include_file != NULL)
            {
                free(output->include_file);
            }
            output->include_file = strings_dup(value);
            if (output->include_file == NULL)
            {
                return -1;
            }
        }
        else if (parse_str_cmp("directory", key))
        {
            if (output->directory != NULL)
            {
                free(output->directory);
                output->directory = NULL;
            }
            char *tmp = strings_dup(value);
            if (tmp == NULL)
            {
                return -1;
            }
            if (*tmp && tmp[strlen(tmp) - 1] != '/')
            {
                output->directory = strings_concat(tmp, "/", 0);
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
                return -1;
            }
        }
        else if (parse_str_cmp("converts", key))
        {
            if (parse_output_converts(output, doc, valuen))
            {
                return -1;
            }
        }
        else if (parse_str_cmp("output-first", key))
        {
            output->order = parse_str_cmp("palettes", value) ?
                OUTPUT_PALETTES_FIRST : OUTPUT_CONVERTS_FIRST;
        }
        else if (parse_str_cmp("prepend-palette-sizes", key))
        {
            output->palette_sizes = parse_str_bool(value);
        }
        else if (parse_str_cmp("const", key))
        {
            output->constant = parse_str_bool(value) ? "const " : "";
        }
        else
        {
            if (output->format != OUTPUT_FORMAT_APPVAR)
            {
                LOG_ERROR("Unknown output option: \'%s\'\n", key);
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }

            if (parse_str_cmp("name", key))
            {
                if (output->appvar.name != NULL)
                {
                    free(output->appvar.name);
                }
                output->appvar.name = strings_dup(value);
                if (output->appvar.name == NULL)
                {
                    return -1;
                }
            }
            else if (parse_str_cmp("archived", key))
            {
                output->appvar.archived = parse_str_bool(value);
            }
            else if (parse_str_cmp("source-init", key))
            {
                output->appvar.init = parse_str_bool(value);
            }
            else if (parse_str_cmp("lut-entries", key))
            {
                output->appvar.lut = parse_str_bool(value);
            }
            else if (parse_str_cmp("lut-entry-size", key))
            {
                int tmpi = strtol(value, NULL, 0);
                if (tmpi != 2 && tmpi != 3)
                {
                    LOG_ERROR("Invalid LUT entry size.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
                output->appvar.entry_size = tmpi;
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
                else if (parse_str_cmp("asm", value))
                {
                    output->appvar.source = APPVAR_SOURCE_ASM;
                }
                else if (parse_str_cmp("none", value))
                {
                    output->appvar.source = APPVAR_SOURCE_NONE;
                }
                else
                {
                    LOG_ERROR("Unknown AppVar source format: \'%s\'\n", value);
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
            }
            else if (parse_str_cmp("compress", key))
            {
                if (parse_str_cmp("zx7", value))
                {
                    output->appvar.compress = COMPRESS_ZX7;
                }
                else if (parse_str_cmp("zx0", value))
                {
                    output->appvar.compress = COMPRESS_ZX0;
                }
                else
                {
                    LOG_ERROR("Invalid compression mode.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }
            }
            else if (parse_str_cmp("header-string", key))
            {
                char *header;
                int header_size;

                if (output->appvar.header != NULL)
                {
                    LOG_ERROR("AppVar header already defined.\n");
                    parser_show_mark_error(keyn->start_mark);
                    return -1;
                }

                header = memory_alloc(valuelen);
                if (header == NULL)
                {
                    return -1;
                }

                header_size =
                    strings_utf8_to_iso8859_1(value, valuelen, header, valuelen);
                if (header_size <= 0)
                {
                    return -1;
                }

                output->appvar.header = header;
                output->appvar.header_size = header_size;
            }
            else
            {
                LOG_ERROR("Unknown appvar output option: \'%s\'\n", key);
                parser_show_mark_error(keyn->start_mark);
                return -1;
            }
        }
    }

    return 0;
}

static int parse_palettes(struct yaml *yaml, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top && item != NULL; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL && parse_palette(yaml, doc, node) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int parse_converts(struct yaml *yaml, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top && item != NULL; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL && parse_convert(yaml, doc, node) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int parse_outputs(struct yaml *yaml, yaml_document_t *doc, yaml_node_t *root)
{
    yaml_node_item_t *item = root->data.sequence.items.start;
    for (; item < root->data.sequence.items.top && item != NULL; ++item)
    {
        yaml_node_t *node = yaml_document_get_node(doc, *item);
        if (node != NULL && parse_output(yaml, doc, node) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int parser_validate(struct yaml *yaml)
{
    uint32_t i;

    for (i = 0; i < yaml->nr_palettes; ++i)
    {
        const char *iname = yaml->palettes[i]->name;
        uint32_t j;

        for (j = 0; j < yaml->nr_palettes; ++j)
        {
            if (j == i)
            {
                continue;
            }

            if (!strcmp(iname, yaml->palettes[j]->name))
            {
                LOG_ERROR("Duplicate palette name \'%s\'", iname);
                return -1;
            }
        }
    }

    for (i = 0; i < yaml->nr_converts; ++i)
    {
        const char *iname = yaml->converts[i]->name;
        uint32_t j;

        for (j = 0; j < yaml->nr_converts; ++j)
        {
            if (j == i)
            {
                continue;
            }

            if (!strcmp(iname, yaml->converts[j]->name))
            {
                LOG_ERROR("Duplicate convert name \'%s\'", iname);
                return -1;
            }
        }
    }

    for (i = 0; i < yaml->nr_converts; ++i)
    {
        struct convert *convert = yaml->converts[i];

        if (convert->style == CONVERT_STYLE_DIRECT)
        {
            if (convert->bpp != BPP_8)
            {
                LOG_ERROR("Convert \'%s\' style does not support \'bpp\' option.\n",
                    convert->name);
                return -1;
            }
            if (convert->palette_name != NULL)
            {
                LOG_ERROR("Convert \'%s\' style does not support \'palette\' option.\n",
                    convert->name);
                return -1;
            }
            if (convert->nr_omit_indices)
            {
                LOG_ERROR("Convert \'%s\' style does not support \'omit-indices\' option.\n",
                    convert->name);
                return -1;
            }
            if (convert->transparent_index > 0)
            {
                LOG_ERROR("Convert \'%s\' style does not support \'transparent-index\' option.\n",
                    convert->name);
                return -1;
            }
            if (convert->palette_offset != 0)
            {
                LOG_ERROR("Convert \'%s\' style does not support \'palette-offset\' option.\n",
                    convert->name);
                return -1;
            }
        }
        else
        {
            if (convert->color_fmt != COLOR_565_RGB)
            {
                LOG_ERROR("Convert \'%s\' style does not support \'color-format\' option.\n",
                    convert->name);
                return -1;
            }
        }
    }

    for (i = 0; i < yaml->nr_outputs; ++i)
    {
        struct output *output = yaml->outputs[i];

        if (output->directory != NULL && output->include_file != NULL)
        {
            char *include_file =
                strings_concat(output->directory, output->include_file, 0);

            if (include_file == NULL)
            {
                return -1;
            }

            free(output->include_file);
            output->include_file = include_file;
        }
    }

    return 0;
}

static int parse_yaml(struct yaml *yaml, yaml_document_t *doc)
{
    yaml_node_t *root = yaml_document_get_root_node(doc);
    yaml_node_pair_t *pair;

    if (root == NULL || root->type != YAML_MAPPING_NODE)
    {
        LOG_ERROR("No valid YAML structures found in input file.\n");
        if (root != NULL)
        {
            parser_show_mark_error(root->start_mark);
        }
        return -1;
    }

    pair = root->data.mapping.pairs.start;
    for (; pair < root->data.mapping.pairs.top; ++pair)
    {
        yaml_node_t *keyn = yaml_document_get_node(doc, pair->key);
        yaml_node_t *valuen = yaml_document_get_node(doc, pair->value);
        int ret;

        if (keyn == NULL || valuen == NULL)
        {
            continue;
        }

        if (parse_str_cmp("palettes", keyn->data.scalar.value))
        {
            ret = parse_palettes(yaml, doc, valuen);
        }
        else if (parse_str_cmp("converts", keyn->data.scalar.value))
        {
            ret = parse_converts(yaml, doc, valuen);
        }
        else if (parse_str_cmp("outputs", keyn->data.scalar.value))
        {
            ret = parse_outputs(yaml, doc, valuen);
        }
        else
        {
            LOG_ERROR("Unknown YAML option: \'%s\'\n", keyn->data.scalar.value);
            parser_show_mark_error(keyn->start_mark);
            ret = -1;
        }

        if (ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

int parser_open(struct yaml *yaml, const char *path)
{
    yaml_parser_t parser;
    yaml_document_t document;
    FILE *fd;
    int ret;

    yaml->path = strings_dup(path);
    if (yaml->path == NULL)
    {
        return -1;
    }

    ret = parser_init(yaml);
    if (ret)
    {
        LOG_ERROR("Could not initialize parser.\n");
        return -1;
    }

    LOG_INFO("Reading file \'%s\'\n", yaml->path);

    fd = fopen(yaml->path, "rt");
    if (fd == NULL)
    {
        LOG_ERROR("Could not open \'%s\': %s\n",
            yaml->path,
            strerror(errno));
        return -1;
    }

    if (!yaml_parser_initialize(&parser))
    {
        LOG_ERROR("Could not initialize YAML parser.\n");
        fclose(fd);
        return -1;
    }

    yaml_parser_set_input_file(&parser, fd);
    if (!yaml_parser_load(&parser, &document))
    {
        parse_show_error(parser.problem, parser.mark);
        yaml_parser_delete(&parser);
        fclose(fd);
        return -1;
    }

    ret = parse_yaml(yaml, &document);

    yaml_document_delete(&document);
    yaml_parser_delete(&parser);

    fclose(fd);

    if (!ret)
    {
        ret = parser_validate(yaml);
    }

    return ret;
}

void parser_close(struct yaml *yaml)
{
    uint32_t i;

    for (i = 0; i < yaml->nr_outputs; ++i)
    {
        output_free(yaml->outputs[i]);
        free(yaml->outputs[i]);
        yaml->outputs[i] = NULL;
    }

    free(yaml->outputs);
    yaml->outputs = NULL;

    for (i = 0; i < yaml->nr_converts; ++i)
    {
        convert_free(yaml->converts[i]);
        free(yaml->converts[i]);
        yaml->converts[i] = NULL;
    }

    free(yaml->converts);
    yaml->converts = NULL;

    for (i = 0; i < yaml->nr_palettes; ++i)
    {
        palette_free(yaml->palettes[i]);
        free(yaml->palettes[i]);
        yaml->palettes[i] = NULL;
    }

    free(yaml->palettes);
    yaml->palettes = NULL;

    free(yaml->path);
    yaml->path = NULL;
}
