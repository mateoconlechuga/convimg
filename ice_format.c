#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "misc.h"
#include "format.h"
#include "logging.h"

// ICE output functions

void ice_print_tilemap_header(output_t *out, const char *name, unsigned int size, unsigned int tile_width, unsigned int tile_height) {
    fprintf(out->txt, "%s | %u bytes\n%u,%u,\"", name, size, tile_width, tile_height);
}

static void ice_open_output(output_t *out, const char *input, bool header) {
    if (input) {
        if (!header) {
            FILE *fd;
            if (!(fd = fopen(input, "w"))) {
                errorf("opening %s for output.", input);
            }
            out->txt = fd;
        }
    }
}

static void ice_close_output(output_t *out, bool header) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    if (!header) {
        if (out->txt) { fclose(out->txt); }
    }
}

static void ice_print_source_header(output_t *out, const char *name) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)name;
    fprintf(out->txt, "| %s\n", convpng_version_string);
    fprintf(out->txt, "| this file contains all the converted graphics\n\n");
}

static void ice_print_header_header(output_t *out, const char *name) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)name;
}

static void ice_print_palette(output_t *out, const char *group_name, liq_palette *pal, const unsigned int pal_len) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    fprintf(out->txt, "%s_pallete | %u bytes\n\"", group_name, pal_len << 1);

    for (unsigned int j = 0; j < pal_len; j++) {
        liq_color *c = &pal->entries[j];
        fprintf(out->txt, "%04X", rgb1555(c->r, c->g, c->b));
    }

    fprintf(out->txt, "\"\n\n");
}

static void ice_print_transparent_index(output_t *out, const char *group_name, const unsigned int index) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    fprintf(out->txt, "%s_transparent_color_index | %u\n\n", group_name, index);
}

static void ice_print_image_source_header(output_t *out, const char *group_header_file_name) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)group_header_file_name;
}

static void ice_print_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size, unsigned int width, unsigned int height) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)i_name;
    (void)tile_num;
    (void)size;
    fprintf(out->txt, "%02X%02X", width, height);
    convpng.allow_newlines = false;
}

static void ice_print_tile_ptrs(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar, unsigned int *offsets) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)i_name;
    (void)num_tiles;
    (void)compressed;
    (void)in_appvar;
    (void)offsets;
}

static void ice_print_compressed_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    fprintf(out->txt, "%s_tile_%u_compressed | %u bytes\n\"", i_name, tile_num, size);
}

static void ice_print_byte(output_t *out, uint8_t byte, bool need_comma) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)need_comma;
    fprintf(out->txt, "%02X", byte);
}

static void ice_print_next_array_line(output_t *out, bool is_long, bool at_end) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)is_long;
    if (at_end && convpng.allow_newlines) {
        fprintf(out->txt, "\"\n\n");
    }
}

static void ice_print_image(output_t *out, uint8_t bpp, const char *i_name, const unsigned int size, const unsigned int width, const unsigned int height) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)bpp;
    if (convpng.output_size) {
        fprintf(out->txt, "%s | %u bytes\n%u,%u,\"", i_name, size, width, height);
    } else {
        fprintf(out->txt, "%s | %u bytes\n\"", i_name, size);
    }
}

static void ice_print_compressed_image(output_t *out, uint8_t bpp, const char *i_name, const unsigned int size) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)bpp;
    fprintf(out->txt, "%s_compressed | %u bytes\n\"", i_name, size);
}

static void ice_print_tiles_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)i_name;
    (void)num_tiles;
    (void)compressed;
    (void)in_appvar;
}

static void ice_print_tiles_ptrs_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)i_name;
    (void)num_tiles;
    (void)compressed;
}

static void ice_print_image_header(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)i_name;
    (void)size;
    (void)width;
    (void)height;
    (void)compressed;
    (void)decompressed_size;
}

static void ice_print_transparent_image_header(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)i_name;
    (void)size;
    (void)width;
    (void)height;
    (void)compressed;
    (void)decompressed_size;
}

static void ice_print_palette_header(output_t *out, const char *name, unsigned int len) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)name;
    (void)len;
}

static void ice_print_palette_fixed_header(output_t *out, fixed_t *colors, int len) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)colors;
    (void)len;
}


static void ice_print_end_header(output_t *out) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
}

static void ice_print_appvar_array(output_t *out, const char *a_name, unsigned int num_images) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)a_name;
    (void)num_images;
}

static void ice_print_appvar_image(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, unsigned int width, unsigned int height, bool table, bool tp_style) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)tp_style;
    (void)width;
    (void)height;
    (void)table;
    if (compressed) {
        fprintf(out->txt, "%s_compressed |\n \"%s\",%u,%u\n\n", i_name, a_name, offset, index);
    } else {
        fprintf(out->txt, "%s |\n \"%s\",%u,%u\n\n", i_name, a_name, offset, index);
    }
}

static void ice_print_appvar_palette(output_t *out, const char *p_name, const char *a_name, unsigned int offset) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    fprintf(out->txt, "%s_palette |\n \"%s\",%u,3\n\n", p_name, a_name, offset);
}

static void ice_print_appvar_load_function_header(output_t *out) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
}

static void ice_print_appvar_export_size(output_t *out, const char *a_name, unsigned int usize, unsigned int csize) {
    (void)out;
    (void)a_name;
    (void)usize;
    (void)csize;
}

static void ice_print_appvar_load_function(output_t *out, const char *a_name, bool has_tilemaps, bool appvar_compressed) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)a_name;
    (void)has_tilemaps;
    (void)appvar_compressed;
}

static void ice_print_appvar_load_function_tilemap(output_t *out, const char *a_name, char *tilemap_name, unsigned int tilemap_size, unsigned int index, bool compressed) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)a_name;
    (void)tilemap_name;
    (void)tilemap_size;
    (void)index;
    (void)compressed;
}

static void ice_print_appvar_load_function_end(output_t *out, bool compressed) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)compressed;
}

static void ice_print_appvar_palette_header(output_t *out, const char *p_name, const char *a_name, unsigned int index, unsigned int offset, unsigned int len, bool table) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)len;
    (void)out;
    (void)offset;
    (void)table;
    (void)p_name;
    (void)a_name;
    (void)index;
}

static void ice_print_include_header(output_t *out, const char *name) {
    if (out == NULL || out->txt == NULL) {
        return;
    }

    (void)out;
    (void)name;
}

const format_t ice_format = {
    .open_output = ice_open_output,
    .close_output = ice_close_output,
    .print_include_header = ice_print_include_header,
    .print_source_header = ice_print_source_header,
    .print_header_header = ice_print_header_header,
    .print_palette = ice_print_palette,
    .print_transparent_index = ice_print_transparent_index,
    .print_image_source_header = ice_print_image_source_header,
    .print_tile = ice_print_tile,
    .print_compressed_tile = ice_print_compressed_tile,
    .print_tile_ptrs = ice_print_tile_ptrs,
    .print_byte = ice_print_byte,
    .print_next_array_line = ice_print_next_array_line,
    .print_image = ice_print_image,
    .print_compressed_image = ice_print_compressed_image,
    .print_tiles_header = ice_print_tiles_header,
    .print_tiles_ptrs_header = ice_print_tiles_ptrs_header,
    .print_image_header = ice_print_image_header,
    .print_transparent_image_header = ice_print_transparent_image_header,
    .print_palette_header = ice_print_palette_header,
    .print_palette_fixed_header = ice_print_palette_fixed_header,
    .print_end_header = ice_print_end_header,
    .print_appvar_array = ice_print_appvar_array,
    .print_appvar_image = ice_print_appvar_image,
    .print_appvar_load_function_header = ice_print_appvar_load_function_header,
    .print_appvar_load_function = ice_print_appvar_load_function,
    .print_appvar_load_function_tilemap = ice_print_appvar_load_function_tilemap,
    .print_appvar_load_function_end = ice_print_appvar_load_function_end,
    .print_appvar_palette_header = ice_print_appvar_palette_header,
    .print_appvar_palette = ice_print_appvar_palette,
    .print_appvar_export_size = ice_print_appvar_export_size,
};
