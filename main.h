#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "appvar.h"
#include "common.h"
#include "libs/libimagequant.h"
#include "libs/lodepng.h"

#define VERSION_MAJOR 6              // version information
#define VERSION_MINOR 8              // minor version

#define MAX_PAL_LEN   256
#define MAX_GROUPS    256            // total number of groups able to create
#define MAX_APPVARS   16

#define ICON_WIDTH    16
#define ICON_HEIGHT   16

#define SIZE_BYTES    2

#define LODEPNG_ERR_OPEN 78

// function prototypes
void add_rgba(uint8_t *pal, size_t size);
void free_rgba(void);

// type of mode for the group
enum mode_t { MODE_C = 0, MODE_ASM, MODE_ICE, MODE_APPVAR };

// no compression, zx7 compression
enum compression_t { COMPRESS_NONE = 0, COMPRESS_ZX7 };

// style modes
enum style_t { STYLE_NONE = 0, STYLE_RLET };

typedef struct {
    FILE *ini;
    FILE *log;
    FILE *all_gfx_c;
    FILE *all_gfx_h;
    unsigned int numgroups;
    unsigned int numappvars;
    unsigned int curline;
    bool output_size;                // output size bytes for images
    bool allow_newlines;             // allow newlines in output
    bool bad_conversion;
    bool icon_zds;
    char *iconc;
    bool using_custom_ini;
    bool using_custom_log;
    bool group_mode;
    char *directory;
    group_t group[MAX_GROUPS];
    appvar_t appvar[MAX_APPVARS];
} convpng_t;

#define OUTPUT_HEADER true
#define OUTPUT_SOURCE false

typedef union {
    struct {
        FILE *c; FILE *h;
    };
    struct {
        FILE *asm; FILE *inc;
    };
    struct {
        FILE *txt;
    };
} output_t;

typedef struct {
    void (*open_output)(output_t *out, const char *input, bool header);
    void (*close_output)(output_t *out, bool header);
    void (*print_include_header)(output_t *out, const char *name);
    void (*print_source_header)(output_t *out, const char *header_file_name);
    void (*print_header_header)(output_t *out, const char *group_name);
    void (*print_palette)(output_t *out, const char *group_name, liq_palette *pal, const unsigned int pal_len);
    void (*print_transparent_index)(output_t *out, const char *group_name, const unsigned int index);
    void (*print_image_source_header)(output_t *out, const char *group_header_file_name);
    void (*print_tile)(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size, unsigned int width, unsigned int height);
    void (*print_compressed_tile)(output_t *out, const char *i_name, unsigned int tile_num, unsigned int size);
    void (*print_tile_ptrs)(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar, unsigned int *offsets);
    void (*print_byte)(output_t *out, uint8_t byte, bool need_comma);
    void (*print_next_array_line)(output_t *out, bool is_long, bool is_end);
    void (*print_image)(output_t *out, uint8_t bpp, const char *i_name, unsigned int size, const unsigned int width, const unsigned int height);
    void (*print_compressed_image)(output_t *out, uint8_t bpp, const char *i_name, unsigned int size);
    void (*print_tiles_header)(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed, bool in_appvar);
    void (*print_tiles_ptrs_header)(output_t *out, const char *i_name, unsigned int num_tiles, bool compressed);
    void (*print_image_header)(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size);
    void (*print_transparent_image_header)(output_t *out, const char *i_name, unsigned int size, unsigned int width, unsigned int height, bool compressed, unsigned int decompressed_size);
    void (*print_palette_header)(output_t *out, const char *name, unsigned int len);
    void (*print_end_header)(output_t *out);

    // appvar functions
    void (*print_appvar_array)(output_t *out, const char *a_name, unsigned int num_images);
    void (*print_appvar_image)(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, unsigned int width, unsigned int height, bool table, bool tp_style);
    void (*print_appvar_palette)(output_t *out, const char *p_name, const char *a_name, unsigned int offset);
    void (*print_appvar_load_function_header)(output_t *out);
    void (*print_appvar_load_function)(output_t *out, const char *a_name, bool has_tilemaps);
    void (*print_appvar_load_function_tilemap)(output_t *out, const char *a_name, char *tilemap_name, unsigned int tilemap_size, unsigned int index, bool compressed);
    void (*print_appvar_load_function_end)(output_t *out);
    void (*print_appvar_palette_header)(output_t *out, const char *p_name, const char *a_name, unsigned int index, unsigned int offset, unsigned int len, bool table);
} format_t;

extern convpng_t convpng;

#endif
