#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "misc.h"
#include "format.h"
#include "logging.h"

// ASM output functions

static void asm_open_output(output_t *out, const char *input, bool header) {
    if (input) {
        FILE *file;
        if (!(file = fopen(input, "w"))) {
            errorf("opening %s for output.", input);
        }

        if (header) {
            out->inc = file;
        } else {
            out->asm = file;
        }
    }
}

static void asm_close_output(output_t *out, bool header) {
    if (header) {
        if (out->inc) { fclose(out->inc); }
    } else {
        if (out->asm) { fclose(out->asm); }
    }
}

static void asm_print_source_header(output_t *out, const char *header_file_name) {
    (void)header_file_name;
    fprintf(out->asm, "; Converted using ConvPNG\n");
}

static void asm_print_header_header(output_t *out, const char *name) {
    fprintf(out->inc, "; Converted using ConvPNG\n");
    fprintf(out->inc, "; This file contains all the graphics for easier inclusion in a project\n");
    fprintf(out->inc, "#include \"%s.asm\"\n", name);
}

static void asm_print_palette(output_t *out, const char *name, liq_palette *pal, const unsigned int pal_len) {
    fprintf(out->asm, "_%s_pal_size equ %u\n", name, pal_len << 1);
    fprintf(out->asm, "_%s_pal:\n", name);
    for (unsigned int j = 0; j < pal_len; j++) {
        liq_color *e = &pal->entries[j];
        fprintf(out->asm, " dw 0%04Xh ; %02u :: rgb(%u,%u,%u)\n", rgb1555(e->r, e->g, e->b), j, e->r, e->g, e->b);
    }
}

static void asm_print_transparent_index(output_t *out, const char *name, const unsigned int index) {
    fprintf(out->inc, "%s_transparent_color_index equ %u\n\n", name, index);
}

static void asm_print_image_source_header(output_t *out, const char *group_header_file_name) {
    (void)group_header_file_name;
    fprintf(out->asm, "; Converted using ConvPNG\n\n");
}

static void asm_print_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size, unsigned int width, unsigned int height) {
	fprintf(out->asm, "_%s_tile_%u_size equ %u\n", i_name, tile_num, size);
    if (convpng.output_size) {
        fprintf(out->asm, "_%s_tile_%u: ; %u bytes\n db %u,%u ; width,height\n db ", i_name, tile_num, size, width, height);
    } else {
        fprintf(out->asm, "_%s_tile_%u: ; %u bytes\n db ", i_name, tile_num, size);
    }
}

static void asm_print_tile_ptrs(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar, unsigned int *offsets) {
    unsigned int i = 0;

    if (compressed) {
        fprintf(out->asm, "_%s_tiles_compressed: ; %u tiles\n", i_name, num_tiles);
        if (in_appvar) {
            for (; i < num_tiles; i++) {
                fprintf(out->asm, " dl %u\n", offsets[i]);
            }
        } else {
            for (; i < num_tiles; i++) {
                fprintf(out->asm, " dl _%s_tile_%u_compressed\n", i_name, i);
            }
        }
    } else {
        fprintf(out->asm, "_%s_tiles: ; %u tiles\n", i_name, num_tiles);
        if (in_appvar) {
            for (; i < num_tiles; i++) {
                fprintf(out->asm, " dl %u\n", offsets[i]);
            }
        } else {
            for (; i < num_tiles; i++) {
                fprintf(out->asm, " dl _%s_tile_%u\n", i_name, i);
            }
        }
    }
}

static void asm_print_compressed_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size) {
    fprintf(out->asm, "_%s_tile_%u_size equ %u\n", i_name, tile_num, size);
    fprintf(out->asm, "_%s_tile_%u_compressed:\n db ", i_name, tile_num);
}

static void asm_print_byte(output_t *out, uint8_t byte, bool need_comma) {
    if (need_comma) {
        fprintf(out->asm, "0%02Xh,", byte);
    } else {
        fprintf(out->asm, "0%02Xh", byte);
    }
}

static void asm_print_next_array_line(output_t *out, bool is_long, bool at_end) {
    if (at_end) {
        fprintf(out->asm, "\n");
    } else {
        if (is_long) {
            fprintf(out->asm, "\n dl ");
        } else {
            fprintf(out->asm, "\n db ");
        }
    }
}

static void asm_print_image(output_t *out, uint8_t bpp, const char *i_name, unsigned int size, const unsigned int width, const unsigned int height) {
    fprintf(out->asm, "; %u bpp image\n", bpp);
	fprintf(out->asm, "_%s_size .equ %u\n", i_name, size);
    if (convpng.output_size) {
	    fprintf(out->asm, "_%s:\n .d%c %u,%u\n db ", i_name, width > 255 || height > 255 ? 'w' : 'b', width, height);
    } else {
        fprintf(out->asm, "_%s:\n db ", i_name);
    }
}

static void asm_print_compressed_image(output_t *out, uint8_t bpp, const char *i_name, unsigned int size) {
    (void)bpp;
    fprintf(out->asm, "_%s_compressed_size equ %u\n", i_name, size);
    fprintf(out->asm, "_%s_compressed:\n db ", i_name);
}

static void asm_print_tiles_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar) {
    (void)compressed;
    (void)num_tiles;
    (void)in_appvar;
    fprintf(out->inc, "#include \"%s\"\n", i_name);
}

static void asm_print_tiles_ptrs_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    (void)compressed;
    (void)num_tiles;
    (void)i_name;
    (void)out;
}

static void asm_print_image_header(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size) {
    (void)compressed;
    (void)width;
    (void)height;
    (void)decompressed_size;
    fprintf(out->inc, "#include \"%s.asm\" ; %u bytes\n", i_name, size);
}

static void asm_print_transparent_image_header(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size) {
    (void)compressed;
    (void)width;
    (void)height;
    (void)decompressed_size;
    fprintf(out->inc, "#include \"%s.asm\" ; %u bytes\n", i_name, size);
}

static void asm_print_palette_header(output_t *out, const char *name, unsigned int len) {
    (void)out;
    (void)name;
    (void)len;
}

static void asm_print_end_header(output_t *out) {
    fprintf(out->inc, "\n");
}

static void asm_print_appvar_array(output_t *out, const char *a_name, unsigned int num_images) {
    fprintf(out->asm, "_%s: ; %u images\n dl ", a_name, num_images);
    fprintf(out->inc, "%s_num equ %u\n\n", a_name, num_images);
}

static void asm_print_appvar_image(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, unsigned int width, unsigned int height, bool table, bool tp_style) {
    (void)tp_style;
    if (table) {
        fprintf(out->asm, "%u", offset);
    }
    if (compressed) {
        if (table) {
            fprintf(out->inc, "%s_compressed equ (%s + %u)\n", i_name, a_name, index * 3);
        }
        fprintf(out->inc, "%s_compressed_offset equ (%u)\n", i_name, offset);
        fprintf(out->inc, "%s_compressed_width equ (%u)\n", i_name, width);
        fprintf(out->inc, "%s_compressed_height equ (%u)\n", i_name, height);
    } else {
        if (table) {
            fprintf(out->inc, "%s equ (%s + %u)\n", i_name, a_name, index * 3);
        }
        fprintf(out->inc, "%s_offset equ (%u)\n", i_name, offset);
        fprintf(out->inc, "%s_width equ (%u)\n", i_name, width);
        fprintf(out->inc, "%s_height equ (%u)\n", i_name, height);
    }
}

static void asm_print_appvar_palette(output_t *out, const char *p_name, const char *a_name, unsigned int offset) {
    (void)p_name;
    (void)a_name;
    fprintf(out->asm, "%u", offset);
}

static void asm_print_appvar_load_function_header(output_t *out) {
    (void)out;
}

static void asm_print_appvar_load_function(output_t *out, const char *a_name, bool has_tilemaps) {
    (void)out;
    (void)a_name;
    (void)has_tilemaps;
}

static void asm_print_appvar_load_function_tilemap(output_t *out, const char *a_name, char *tilemap_name, unsigned int tilemap_size, unsigned int index, bool compressed) {
    (void)out;
    (void)a_name;
    (void)tilemap_name;
    (void)tilemap_size;
    (void)index;
    (void)compressed;
}

static void asm_print_appvar_load_function_end(output_t *out) {
    (void)out;
}

static void asm_print_appvar_palette_header(output_t *out, const char *p_name, const char *a_name, unsigned int index, unsigned int offset, unsigned int len, bool table) {
    fprintf(out->inc, "sizeof_%s_pal equ %u\n", p_name, len * 2);
    if (table) {
        fprintf(out->inc, "%s_pal equ (%s + %u)\n", p_name, a_name, index * 3);
    }
    fprintf(out->inc, "%s_pal_offset equ (%u)\n", p_name, offset);
}

static void asm_print_include_header(output_t *out, const char *name) {
    (void)out;
    (void)name;
}

const format_t asm_format = {
    .open_output = asm_open_output,
    .close_output = asm_close_output,
    .print_include_header = asm_print_include_header,
    .print_source_header = asm_print_source_header,
    .print_header_header = asm_print_header_header,
    .print_palette = asm_print_palette,
    .print_transparent_index = asm_print_transparent_index,
    .print_image_source_header = asm_print_image_source_header,
    .print_tile = asm_print_tile,
    .print_compressed_tile = asm_print_compressed_tile,
    .print_tile_ptrs = asm_print_tile_ptrs,
    .print_byte = asm_print_byte,
    .print_next_array_line = asm_print_next_array_line,
    .print_image = asm_print_image,
    .print_compressed_image = asm_print_compressed_image,
    .print_tiles_header = asm_print_tiles_header,
    .print_tiles_ptrs_header = asm_print_tiles_ptrs_header,
    .print_image_header = asm_print_image_header,
    .print_transparent_image_header = asm_print_transparent_image_header,
    .print_palette_header = asm_print_palette_header,
    .print_end_header = asm_print_end_header,
    .print_appvar_array = asm_print_appvar_array,
    .print_appvar_image = asm_print_appvar_image,
    .print_appvar_load_function_header = asm_print_appvar_load_function_header,
    .print_appvar_load_function = asm_print_appvar_load_function,
    .print_appvar_load_function_tilemap = asm_print_appvar_load_function_tilemap,
    .print_appvar_load_function_end = asm_print_appvar_load_function_end,
    .print_appvar_palette_header = asm_print_appvar_palette_header,
    .print_appvar_palette = asm_print_appvar_palette,
};
