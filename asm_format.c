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
        fclose(out->inc);
    } else {
        fclose(out->asm);
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
    fprintf(out->inc, "; assembler defines\n");
    fprintf(out->inc, "#define db .db\n#define dw .dw\n#define dl .dl\n\n");
    fprintf(out->inc, "#include \"%s.asm\"\n", group_name);
}

static void asm_print_palette(output_t *out, const char *group_name, liq_palette *pal, const unsigned int pal_len) {
    fprintf(out->asm, "_%s_pal_size equ %u\n", group_name, pal_len << 1);
    fprintf(out->asm, "_%s_pal:\n", group_name);
    for (unsigned int j = 0; j < pal_len; j++) {
        liq_color *e = &pal->entries[j];
        fprintf(out->asm, " dw 0%04Xh ; %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(e->r, e->g, e->b), j, e->r, e->g, e->b, e->a);
    }
}

static void asm_print_transparent_index(output_t *out, const char *group_name, const unsigned int index) {
    fprintf(out->inc, "%s_transparent_color_index equ %u\n\n", group_name, index);
}

static void asm_print_image_source_header(output_t *out, const char *group_header_file_name) {
    (void)group_header_file_name;
    fprintf(out->asm, "; Converted using ConvPNG\n\n");
}

static void asm_print_tile(output_t *out, const char *image_name, unsigned int tile_num, unsigned int size, uint8_t width, uint8_t height) {
    fprintf(out->asm, "_%s_tile_%u_data: ; %u bytes\n db %u,%u ; width,height\n db ", image_name, tile_num, size, width, height);
}

static void asm_print_tile_ptrs(output_t *out, const char *image_name, unsigned int num_tiles, bool compressed) {
    unsigned int i = 0;

    if (compressed) {
        fprintf(out->asm, "_%s_tiles_compressed: ; %u tiles\n", image_name, num_tiles);
        for (; i < num_tiles; i++) {
            fprintf(out->asm, " dl _%s_tile_%u_compressed\n", image_name, i);
        }
    } else {
        fprintf(out->asm, "_%s_tiles_data: ; %u tiles\n", image_name, num_tiles);
        for (; i < num_tiles; i++) {
            fprintf(out->asm, " dl _%s_tile_%u_data\n", image_name, i);
        }
    }
}

static void asm_print_compressed_tile(output_t *out, const char *image_name, unsigned int tile_num, unsigned int size) {
    fprintf(out->asm, "_%s_tile_%u_size equ %u\n", image_name, tile_num, size);
    fprintf(out->asm, "_%s_tile_%u_compressed:\n db ", image_name, tile_num);
}

static void asm_print_byte(output_t *out, uint8_t byte, bool need_comma) {
    if (need_comma) {
        fprintf(out->asm, "0%02Xh,", byte);
    } else {
        fprintf(out->asm, "0%02Xh", byte);
    }
}

static void asm_print_next_array_line(output_t *out) {
    fprintf(out->asm, "\n db ");
}

static void asm_print_terminate_array(output_t *out) {
    (void)out;
}

static void asm_print_image(output_t *out, uint8_t bpp, const char *image_name, unsigned int size, const uint8_t width, const uint8_t height) {
    fprintf(out->asm, "; %u bpp image\n_%s: ; %u bytes\n db %u,%u\n db ", bpp, image_name, size, width, height);
}

static void asm_print_compressed_image(output_t *out, uint8_t bpp, const char *image_name, unsigned int size) {
    (void)bpp;
    fprintf(out->asm, "_%s_compressed_size equ %u\n", image_name, size); 
    fprintf(out->asm, "_%s_compressed:\n db ", image_name);
}

static void asm_print_tiles_header(output_t *out, const char *image_name, unsigned int num_tiles, bool compressed) {
    (void)compressed;
    (void)num_tiles;
    fprintf(out->inc, "#include \"%s\"\n", image_name);
}

static void asm_print_tiles_ptrs_header(output_t *out, const char *image_name, unsigned int num_tiles, bool compressed) {
    (void)compressed;
    (void)num_tiles;
    (void)image_name;
    (void)out;
}

static void asm_print_image_header(output_t *out, const char *image_name, unsigned int size, bool compressed) {
    (void)compressed;
    fprintf(out->h, "#include \"%s.asm\" ; %u bytes\n", image_name, size);
}

static void asm_print_palette_header(output_t *out, const char *name, uint8_t len) {
    (void)out;
    (void)name;
    (void)len;
}

static void asm_print_end_header(output_t *out) {
    fprintf(out->h, "\n#endif\n");
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
    .print_terminate_array = asm_print_terminate_array,
    .print_image = asm_print_image,
    .print_compressed_image = asm_print_compressed_image,
    .print_tiles_header = asm_print_tiles_header,
    .print_tiles_ptrs_header = asm_print_tiles_ptrs_header,
    .print_image_header = asm_print_image_header,
    .print_palette_header = asm_print_palette_header,
    .print_end_header = asm_print_end_header
};


