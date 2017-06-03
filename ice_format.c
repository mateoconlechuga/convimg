#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "misc.h"
#include "format.h"
#include "logging.h"

// ICE output functions

static void ice_open_output(output_t *out, const char *input, bool header) {
    if (input) {
        if (!header) {
            FILE *file;
            if (!(file = fopen(input, "w"))) {
                errorf("opening %s for output.", input);
            }
            out->txt = file;
        }
    }
}

static void ice_close_output(output_t *out, bool header) {
    if (!header) {
        if (out->txt) { fclose(out->txt); }
    }
}

static void ice_print_source_header(output_t *out, const char *header_file_name) {
    (void)header_file_name;
    fprintf(out->txt, "Converted using ConvPNG\n");
    fprintf(out->txt, "This file contains all the converted graphics\n\n");
}

static void ice_print_header_header(output_t *out, const char *group_name) {
    (void)out;
    (void)group_name;
}

static void ice_print_palette(output_t *out, const char *group_name, liq_palette *pal, const unsigned int pal_len) {
    fprintf(out->txt, "%s_pallete | %u bytes\n\"", group_name, pal_len << 1);
    
    for (unsigned int j = 0; j < pal_len; j++) {
        liq_color *c = &pal->entries[j];
        fprintf(out->txt, "%04X", rgb1555(c->r, c->g, c->b));
    }
    
    fprintf(out->txt, "\"\n\n");
}

static void ice_print_transparent_index(output_t *out, const char *group_name, const unsigned int index) {
    fprintf(out->txt, "%s_transparent_color_index | %u\n\n", group_name, index);
}

static void ice_print_image_source_header(output_t *out, const char *group_header_file_name) {
    (void)out;
    (void)group_header_file_name;
}

static void ice_print_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size, uint8_t width, uint8_t height) {
    fprintf(out->txt, "%s_tile_%u | %u bytes\n%u,%u,\"", i_name, tile_num, size, width, height);
}

static void ice_print_tile_ptrs(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    (void)out;
    (void)i_name;
    (void)num_tiles;
    (void)compressed;
}

static void ice_print_compressed_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size) {
    fprintf(out->txt, "%s_tile_%u_compressed | %u bytes\n\"", i_name, tile_num, size);
}

static void ice_print_byte(output_t *out, uint8_t byte, bool need_comma) {
    (void)need_comma;
    fprintf(out->txt, "%02X", byte);
}

static void ice_print_next_array_line(output_t *out, bool is_long, bool at_end) {
    (void)is_long;
    if (at_end) {
        fprintf(out->txt, "\"\n\n");
    }
}

static void ice_print_image(output_t *out, uint8_t bpp, const char *i_name, const unsigned int size, const uint8_t width, const uint8_t height) {
    (void)bpp;
    fprintf(out->txt, "%s | %u bytes\n%u,%u,\"", i_name, size, width, height);
}

static void ice_print_compressed_image(output_t *out, uint8_t bpp, const char *i_name, const unsigned int size) {
    (void)bpp;
    fprintf(out->txt, "%s_compressed | %u bytes\n\"", i_name, size);
}

static void ice_print_tiles_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    (void)out;
    (void)i_name;
    (void)num_tiles;
    (void)compressed;
}

static void ice_print_tiles_ptrs_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    (void)out;
    (void)i_name;
    (void)num_tiles;
    (void)compressed;
}

static void ice_print_image_header(output_t *out, const char *i_name, unsigned int size, bool compressed) {
    (void)out;
    (void)i_name;
    (void)size;
    (void)compressed;
}

static void ice_print_transparent_image_header(output_t *out, const char *i_name, unsigned int size, bool compressed) {
    (void)out;
    (void)i_name;
    (void)size;
    (void)compressed;
}

static void ice_print_palette_header(output_t *out, const char *name, unsigned int len) {
    (void)out;
    (void)name;
    (void)len;
}

static void ice_print_end_header(output_t *out) {
    (void)out;
}

static void ice_print_appvar_array(output_t *out, const char *a_name, unsigned int num_images) {
    (void)out;
    (void)a_name;
    (void)num_images;
}

static void ice_print_appvar_image(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, bool tp_style) {
    (void)out;
    (void)a_name;
    (void)i_name;
    (void)offset;
    (void)index;
    (void)compressed;
    (void)tp_style;
}

static void ice_print_appvar_palette(output_t *out, unsigned int offset) {
    (void)out;
    (void)offset;
}

static void ice_print_appvar_load_function_header(output_t *out) {
    (void)out;
}

static void ice_print_appvar_load_function(output_t *out, const char *a_name) {
    (void)out;
    (void)a_name;
}

static void ice_print_appvar_palette_header(output_t *out, const char *p_name, const char *a_name, unsigned int index, unsigned int len) {
    (void)out;
    (void)p_name;
    (void)a_name;
    (void)index;
    (void)len;
}

const format_t ice_format = {
    .open_output = ice_open_output,
    .close_output = ice_close_output,
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
    .print_end_header = ice_print_end_header,
    .print_appvar_array = ice_print_appvar_array,
    .print_appvar_image = ice_print_appvar_image,
    .print_appvar_load_function_header = ice_print_appvar_load_function_header,
    .print_appvar_load_function = ice_print_appvar_load_function,
    .print_appvar_palette_header = ice_print_appvar_palette_header,
    .print_appvar_palette = ice_print_appvar_palette,
};

