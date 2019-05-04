#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "misc.h"
#include "format.h"
#include "logging.h"

// C format output functions

static void c_open_output(output_t *out, const char *input, bool header) {
    if (input) {
        FILE *fd;
        if ((fd = fopen(input, "w")) == NULL || out == NULL) {
            errorf("opening %s for output.", input);
        }

        if (header) {
            out->h = fd;
        } else {
            out->c = fd;
        }
    }
}

static void c_close_output(output_t *out, bool header) {
    if (header) {
        if (out->h) { fclose(out->h); }
    } else {
        if (out->c) { fclose(out->c); }
    }
}

static void c_print_source_header(output_t *out, const char *name) {
    fprintf(out->c, "// %s\n", convpng_version_string);
    fprintf(out->c, "#include <stdint.h>\n");
    fprintf(out->c, "#include \"%s\"\n\n", name);
}

static void c_print_header_header(output_t *out, const char *name) {
    fprintf(out->h, "// %s\n", convpng_version_string);
    fprintf(out->h, "// this file contains all the graphics sources for easy inclusion in a project\n");
    fprintf(out->h, "#ifndef __%s__\n#define __%s__\n", name, name);
    fprintf(out->h, "#include <stdint.h>\n\n");
}

static void c_print_palette(output_t *out, const char *name, liq_palette *pal, const unsigned int pal_len) {
    fprintf(out->c, "uint16_t %s_pal[%u] = {\n", name, pal_len);

    for (unsigned int j = 0; j < pal_len; j++) {
        liq_color *e = &pal->entries[j];
        fprintf(out->c, " 0x%04X,  // %02u :: rgb(%u,%u,%u)\n", rgb1555(e->r, e->g, e->b), j, e->r, e->g, e->b);
    }
    fprintf(out->c, "};\n");
}

static void c_print_transparent_index(output_t *out, const char *name, const unsigned int index) {
    fprintf(out->h, "#define %s_transparent_color_index %u\n\n", name, index);
}

static void c_print_image_source_header(output_t *out, const char *name) {
    fprintf(out->c, "// %s\n", convpng_version_string);
    fprintf(out->c, "#include <stdint.h>\n");
    fprintf(out->c, "#include \"%s\"\n\n", name);
}

static void c_print_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size, unsigned int width, unsigned int height) {
    if (convpng.output_size) {
        fprintf(out->c, "uint8_t %s_tile_%u_data[%u] = {\n %u,\t// tile_width\n %u,\t// tile_height\n ",
                i_name, tile_num, size, width, height);
    } else {
        fprintf(out->c, "uint8_t %s_tile_%u_data[%u] = {\n ",
                i_name, tile_num, size);
    }
}

static void c_print_tile_ptrs(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar, unsigned int *offsets) {
    unsigned int i = 0;

    if (compressed) {
        fprintf(out->c, "uint8_t *%s_tiles_compressed[%u] = {\n", i_name, num_tiles);
        if (in_appvar) {
            for (; i < num_tiles; i++) {
                fprintf(out->c, " (uint8_t*)%u,\n", offsets[i]);
            }
        } else {
            for (; i < num_tiles; i++) {
                fprintf(out->c, " %s_tile_%u_compressed,\n", i_name, i);
            }
        }
    } else {
        fprintf(out->c, "uint8_t *%s_tiles_data[%u] = {\n", i_name, num_tiles);
        if (in_appvar) {
            for (; i < num_tiles; i++) {
                fprintf(out->c, " (uint8_t*)%u,\n", offsets[i]);
            }
        } else {
            for (; i < num_tiles; i++) {
                fprintf(out->c, " %s_tile_%u_data,\n", i_name, i);
            }
        }
    }
    fprintf(out->c, "};\n");
}

static void c_print_compressed_tile(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size) {
    fprintf(out->c, "uint8_t %s_tile_%u_compressed[%u] = {\n ", i_name, tile_num, size);
}

static void c_print_byte(output_t *out, uint8_t byte, bool need_comma) {
    (void)need_comma;
    fprintf(out->c, "0x%02X,", byte);
}

static void c_print_next_array_line(output_t *out, bool is_long, bool at_end) {
    (void)is_long;
    if (at_end) {
        fprintf(out->c, "\n};\n");
    } else {
        fprintf(out->c, "\n ");
    }
}

static void c_print_image(output_t *out, uint8_t bpp, const char *i_name, unsigned int size, const unsigned int width, const unsigned int height) {
    if (convpng.output_size) {
        fprintf(out->c, "// %u bpp image\nuint8_t %s_data[%u] = {\n %u,%u,  // width,height\n ",
                bpp, i_name, size, width, height);
    } else {
        fprintf(out->c, "// %u bpp image\nuint8_t %s_data[%u] = {\n ",
                bpp, i_name, size);
    }
}

static void c_print_compressed_image(output_t *out, uint8_t bpp, const char *i_name, unsigned int size) {
    fprintf(out->c, "// %u bpp image\nuint8_t %s_compressed[%u] = {\n ", bpp, i_name, size);
}

static void c_print_tiles_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar) {
    unsigned int i = 0;
    if (compressed) {
        if (in_appvar) {
            for (; i < num_tiles; i++) {
                fprintf(out->h, "#define %s_tile_%u_compressed ((gfx_sprite_t*)%s_tiles_compressed[%u])\n", i_name, i, i_name, i);
            }
        } else {
            for (; i < num_tiles; i++) {
                fprintf(out->h, "extern uint8_t %s_tile_%u_compressed[];\n", i_name, i);
            }
        }
    } else {
        if (in_appvar) {
            for (; i < num_tiles; i++) {
                fprintf(out->h, "#define %s_tile_%u ((gfx_sprite_t*)%s_tiles_data[%u])\n", i_name, i, i_name, i);
            }
        } else {
            for (; i < num_tiles; i++) {
                fprintf(out->h,"extern uint8_t %s_tile_%u_data[];\n", i_name, i);
                fprintf(out->h, "#define %s_tile_%u ((gfx_sprite_t*)%s_tile_%u_data)\n", i_name, i, i_name, i);
            }
        }
    }
}

static void c_print_tiles_ptrs_header(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed) {
    if (compressed) {
        fprintf(out->h, "#define %s_tiles_num %u\n", i_name, num_tiles);
        fprintf(out->h, "extern uint8_t *%s_tiles_compressed[%u];\n", i_name, num_tiles);
    } else {
        fprintf(out->h, "#define %s_tiles_num %u\n", i_name, num_tiles);
        fprintf(out->h, "extern uint8_t *%s_tiles_data[%u];\n", i_name, num_tiles);
        fprintf(out->h, "#define %s_tiles ((gfx_sprite_t**)%s_tiles_data)\n", i_name, i_name);
    }
}

static void c_print_image_header(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size) {
    fprintf(out->h, "#define %s_width %u\n", i_name, width);
    fprintf(out->h, "#define %s_height %u\n", i_name, height);
    if (compressed) {
        fprintf(out->h, "#define %s_size %u\n", i_name, decompressed_size);
        fprintf(out->h, "extern uint8_t %s_compressed[%u];\n", i_name, size);
    } else {
        fprintf(out->h, "#define %s_size %u\n", i_name, size);
        fprintf(out->h, "extern uint8_t %s_data[%u];\n", i_name, size);
        fprintf(out->h, "#define %s ((gfx_sprite_t*)%s_data)\n", i_name, i_name);
    }
}

static void c_print_transparent_image_header(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size) {
    fprintf(out->h, "#define %s_width %u\n", i_name, width);
    fprintf(out->h, "#define %s_height %u\n", i_name, height);
    if (compressed) {
        fprintf(out->h, "#define %s_size %u\n", i_name, decompressed_size);
        fprintf(out->h, "extern uint8_t %s_compressed[%u];\n", i_name, size);
    } else {
        fprintf(out->h, "#define %s_size %u\n", i_name, size);
        fprintf(out->h, "extern uint8_t %s_data[%u];\n", i_name, size);
        fprintf(out->h, "#define %s ((gfx_rletsprite_t*)%s_data)\n", i_name, i_name);
    }
}

static void c_print_palette_header(output_t *out, const char *name, unsigned int len) {
    fprintf(out->h, "#define sizeof_%s_pal %u\n", name, len * 2);
    fprintf(out->h, "extern uint16_t %s_pal[%u];\n", name, len);
}

static void c_print_end_header(output_t *out) {
    fprintf(out->h, "\n#endif\n");
}

static void c_print_appvar_array(output_t *out, const char *a_name, unsigned int num_images) {
    fprintf(out->c, "uint8_t *%s[%u] = {\n ", a_name, num_images);
    fprintf(out->h,"#include <stdbool.h>\n\n");
    fprintf(out->h, "#define %s_num %u\n\n", a_name, num_images);
    fprintf(out->h, "extern uint8_t *%s[%u];\n", a_name, num_images);
}

static void c_print_appvar_image(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, unsigned int width, unsigned int height, bool table, bool tp_style) {
    (void)table;
    fprintf(out->h, "#define %s_width %u\n", i_name, width);
    fprintf(out->h, "#define %s_height %u\n", i_name, height);
    const char *s = "gfx_sprite_t";
    if (tp_style) {
        s = "gfx_rletsprite_t";
    }
    fprintf(out->c, "(uint8_t*)%u,", offset);
    if (compressed) {
        fprintf(out->h, "#define %s_compressed ((%s*)%s[%u])\n", i_name, s, a_name, index);
    } else {
        fprintf(out->h, "#define %s ((%s*)%s[%u])\n", i_name, s, a_name, index);
    }
}

static void c_print_appvar_palette(output_t *out, const char *p_name, const char *a_name, unsigned int offset) {
    (void)p_name;
    (void)a_name;
    fprintf(out->c, "(uint8_t*)%u,", offset);
}

static void c_print_appvar_load_function_header(output_t *out) {
    fprintf(out->c, "#include <fileioc.h>\n");
}

static void c_print_appvar_export_size(output_t *out, const char *a_name, unsigned int usize, unsigned int csize) {
    fprintf(out->h, "#define %s_uncompressed_size %u\n", a_name, usize);
    fprintf(out->h, "#define %s_compressed_size %u\n", a_name, csize);
}

static void c_print_appvar_load_function(output_t *out, const char *a_name, bool has_tilemaps, bool appvar_compressed) {
    (void)has_tilemaps;
    if (appvar_compressed) {
        fprintf(out->h, "bool %s_init(void *decompressed_addr);\n", a_name);
        fprintf(out->c, "\nbool %s_init(void *decompressed_addr) {\n", a_name);
        fprintf(out->c, "    unsigned int data, i;\n\n");
        fprintf(out->c, "    data = (unsigned int)decompressed_addr - (unsigned int)%s[0];\n", a_name);
        fprintf(out->c, "    for (i = 0; i < %s_num; i++) {\n", a_name);
        fprintf(out->c, "        %s[i] += data;\n", a_name);
        fprintf(out->c, "    }\n\n");
    } else {
        fprintf(out->h, "bool %s_init(void);\n", a_name);
        fprintf(out->c, "\nbool %s_init(void) {\n", a_name);
        fprintf(out->c, "    unsigned int data, i;\n");
        fprintf(out->c, "    ti_var_t appvar;\n\n");
        fprintf(out->c, "    ti_CloseAll();\n\n");
        fprintf(out->c, "    appvar = ti_Open(\"%s\", \"r\");\n", a_name);
        fprintf(out->c, "    data = (unsigned int)ti_GetDataPtr(appvar) - (unsigned int)%s[0];\n", a_name);
        fprintf(out->c, "    for (i = 0; i < %s_num; i++) {\n", a_name);
        fprintf(out->c, "        %s[i] += data;\n", a_name);
        fprintf(out->c, "    }\n\n");
        fprintf(out->c, "    ti_CloseAll();\n\n");
    }
}

static void c_print_appvar_load_function_tilemap(output_t *out, const char *a_name, char *tilemap_name, unsigned int tilemap_size, unsigned int index, bool compressed) {
    (void)tilemap_size;
    if (compressed) {
        fprintf(out->c, "    data = (unsigned int)%s[%u] - (unsigned int)%s_tiles_compressed[0];\n", a_name, index, tilemap_name);
        fprintf(out->c, "    for (i = 0; i < %s_tiles_num; i++) {\n", tilemap_name);
        fprintf(out->c, "        %s_tiles_compressed[i] += data;\n", tilemap_name);
        fprintf(out->c, "    }\n\n");
    } else {
        fprintf(out->c, "    data = (unsigned int)%s[%u] - (unsigned int)%s_tiles_data[0];\n", a_name, index, tilemap_name);
        fprintf(out->c, "    for (i = 0; i < %s_tiles_num; i++) {\n", tilemap_name);
        fprintf(out->c, "        %s_tiles_data[i] += data;\n", tilemap_name);
        fprintf(out->c, "    }\n\n");
    }
}

static void c_print_appvar_load_function_end(output_t *out, bool compressed) {
    if (compressed) {
        fprintf(out->c, "    return true;\n");
    } else {
        fprintf(out->c, "    return (bool)appvar;\n");
    }
    fprintf(out->c, "}\n");
}

static void c_print_appvar_palette_header(output_t *out, const char *p_name, const char *a_name, unsigned int index, unsigned int offset, unsigned int len, bool table) {
    (void)offset;
    (void)table;
    fprintf(out->h, "#define sizeof_%s_pal %u\n", p_name, len * 2);
    fprintf(out->h, "#define %s_pal ((uint16_t*)%s[%u])\n", p_name, a_name, index);
}

static void c_print_include_header(output_t *out, const char *name) {
    fprintf(out->c, "#include \"%s.h\"\n", name);
}

const format_t c_format = {
    .open_output = c_open_output,
    .close_output = c_close_output,
    .print_include_header = c_print_include_header,
    .print_source_header = c_print_source_header,
    .print_header_header = c_print_header_header,
    .print_palette = c_print_palette,
    .print_transparent_index = c_print_transparent_index,
    .print_image_source_header = c_print_image_source_header,
    .print_tile = c_print_tile,
    .print_compressed_tile = c_print_compressed_tile,
    .print_tile_ptrs = c_print_tile_ptrs,
    .print_byte = c_print_byte,
    .print_next_array_line = c_print_next_array_line,
    .print_image = c_print_image,
    .print_compressed_image = c_print_compressed_image,
    .print_tiles_header = c_print_tiles_header,
    .print_tiles_ptrs_header = c_print_tiles_ptrs_header,
    .print_image_header = c_print_image_header,
    .print_transparent_image_header = c_print_transparent_image_header,
    .print_palette_header = c_print_palette_header,
    .print_end_header = c_print_end_header,
    .print_appvar_array = c_print_appvar_array,
    .print_appvar_image = c_print_appvar_image,
    .print_appvar_load_function_header = c_print_appvar_load_function_header,
    .print_appvar_load_function = c_print_appvar_load_function,
    .print_appvar_load_function_tilemap = c_print_appvar_load_function_tilemap,
    .print_appvar_load_function_end = c_print_appvar_load_function_end,
    .print_appvar_palette_header = c_print_appvar_palette_header,
    .print_appvar_palette = c_print_appvar_palette,
    .print_appvar_export_size = c_print_appvar_export_size,
};
