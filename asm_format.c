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
            out->h = file;
        } else {
            out->c = file;
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

static void asm_print_header_header(output_t *out, const char *group_name) {
    fprintf(out->inc, "; Converted using ConvPNG\n");
    fprintf(out->inc, "; This file contains all the graphics for easier inclusion in a project\n");
    fprintf(out->inc, "#ifndef __%s__\n#define __%s__\n\n", group_name, group_name);
    fprintf(out->inc, "#include \"%s.asm\"\n", group_name);
}

static void asm_print_palette(output_t *out, const char *group_name, liq_palette *pal, const unsigned int pal_len) {
    fprintf(out->asm, "_%s_pal_size .equ %u\n", group_name, pal_len << 1);
    fprintf(out->asm, "_%s_pal:\n", group_name);
    for (unsigned int j = 0; j < pal_len; j++) {
        liq_color *e = &pal->entries[j];
        fprintf(out->asm, " .dw 0%04Xh ; %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(e->r, e->g, e->b), j, e->r, e->g, e->b, e->a);
    }
}

static void asm_print_transparent_index(output_t *out, const char *group_name, const unsigned int index) {
    fprintf(out->inc, "%s_transparent_color_index .equ %u\n\n", group_name, index);
}

static void asm_print_image_source_header(output_t *out, const char *group_header_file_name) {
    (void)group_header_file_name;
    fprintf(out->asm, "; Converted using ConvPNG\n\n");
}

static void asm_print_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size, uint8_t width, uint8_t height) {
	fprintf(out->asm, "_%s_tile_%u_size .equ %u\n", i_name, tile_num, size);
    fprintf(out->asm, "_%s_tile_%u: ; %u bytes\n db %u,%u ; width,height\n db ", i_name, tile_num, size, width, height);
}

static void asm_print_tile_ptrs(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    unsigned int i = 0;

    if (compressed) {
        fprintf(out->asm, "_%s_tiles_compressed: ; %u tiles\n", i_name, num_tiles);
        for (; i < num_tiles; i++) {
            fprintf(out->asm, " .dl _%s_tile_%u_compressed\n", i_name, i);
        }
    } else {
        fprintf(out->asm, "_%s_tiles: ; %u tiles\n", i_name, num_tiles);
        for (; i < num_tiles; i++) {
            fprintf(out->asm, " .dl _%s_tile_%u\n", i_name, i);
        }
    }
}

static void asm_print_compressed_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size) {
    fprintf(out->asm, "_%s_tile_%u_size .equ %u\n", i_name, tile_num, size);
    fprintf(out->asm, "_%s_tile_%u_compressed:\n .db ", i_name, tile_num);
}

static void asm_print_byte(output_t *out, uint8_t byte, bool need_comma) {
    if (need_comma) {
        fprintf(out->asm, "0%02Xh,", byte);
    } else {
        fprintf(out->asm, "0%02Xh", byte);
    }
}

static void asm_print_next_array_line(output_t *out, bool at_end) {
    if (at_end) {
        fprintf(out->asm, "\n");
    } else {
        fprintf(out->asm, "\n .db ");
    }
}

static void asm_print_image(output_t *out, uint8_t bpp, const char *i_name, unsigned int size, const uint8_t width, const uint8_t height) {
    fprintf(out->asm, "; %u bpp image\n", bpp);
	fprintf(out->asm, "_%s_size .equ %u\n", i_name, size);
	fprintf(out->asm, "_%s:\n .db %u,%u\n .db ", i_name, width, height);
}

static void asm_print_compressed_image(output_t *out, uint8_t bpp, const char *i_name, unsigned int size) {
    (void)bpp;
    fprintf(out->asm, "_%s_compressed_size equ %u\n", i_name, size); 
    fprintf(out->asm, "_%s_compressed:\n .db ", i_name);
}

static void asm_print_tiles_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    (void)compressed;
    (void)num_tiles;
    fprintf(out->inc, "#include \"%s\"\n", i_name);
}

static void asm_print_tiles_ptrs_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    (void)compressed;
    (void)num_tiles;
    (void)i_name;
    (void)out;
}

static void asm_print_image_header(output_t *out, const char *i_name, unsigned int size, bool compressed) {
    (void)compressed;
    fprintf(out->h, "#include \"%s.asm\" ; %u bytes\n", i_name, size);
}

static void asm_print_transparent_image_header(output_t *out, const char *i_name, unsigned int size, bool compressed) {
    (void)compressed;
    fprintf(out->h, "#include \"%s.asm\" ; %u bytes\n", i_name, size);
}

static void asm_print_palette_header(output_t *out, const char *name, unsigned int len) {
    (void)out;
    (void)name;
    (void)len;
}

static void asm_print_end_header(output_t *out) {
    fprintf(out->h, "\n#endif\n");
}

static void asm_print_appvar_array(output_t *out, const char *a_name, unsigned int num_images) {
    (void)out;
    (void)a_name;
    (void)num_images;
}

static void asm_print_appvar_image(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, bool tp_style) {
    (void)out;
    (void)a_name;
    (void)i_name;
    (void)offset;
    (void)index;
    (void)compressed;
    (void)tp_style;
}

const format_t asm_format = {
    .open_output = asm_open_output,
    .close_output = asm_close_output,
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
};


