#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "libs/libimagequant.h"
#include "libs/lodepng.h"

#define VERSION_MAJOR 6              // version information
#define VERSION_MINOR 0              // minor version

#define MAX_PAL_LEN   256 
#define NUM_GROUPS    256            // total number of groups able to create

#define ICON_WIDTH    16
#define ICON_HEIGHT   16

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

typedef struct s_st {
    char *in;                        // name of image on disk
    char *outc;                      // output file name (.c, .asm)
    char *name;                      // name of image
    unsigned int compression;        // compression information
    unsigned int style;              // output style
    bool convert_to_tilemap;         // should we convert to a tilemap?
    bool create_tilemap_ptrs;        // should we create an array of pointers to the tiles?
    unsigned int numtiles;           // number of tiles in the image
} image_t;

typedef struct g_st {
    char *name;                      // name of the group file
    char *palette;                   // custom palette file name
    unsigned palette_length;         // custom palette length
    bool palette_fixed_length;       // bool to check if forced palette size
    image_t **image;                 // pointer to array of images
    unsigned int numimages;          // number of images in the group
    char *outh;                      // output main .inc or .h file
    char *outc;                      // output main .asm or .c file
    int tindex;                      // index to use as the transparent color
    uint16_t tcolor_converted;       // converted transparent color
    liq_color tcolor;                // apparently more about the transparent color
    bool use_tcolor;                 // bool to tell if to compute transparent color
    bool use_tindex;                 // bool to use a new index
    unsigned int mode;               // either asm or c style conversion
    unsigned int style;              // Style of ouput conversion
    unsigned int compression;        // compression type
    unsigned int tile_width;         // for use creating tilemaps
    unsigned int tile_height;        // for use creating tilemaps
    unsigned int tile_size;          // tile_height * tile_width + 2
    unsigned int total_tiles;        // number of tiles in the image
    bool convert_to_tilemap;         // should we convert to a tilemap?
    bool create_tilemap_ptrs;        // should we create an array of pointers to the tiles?
    bool output_palette_image;       // does the user want an image of the palette?
    bool output_palette_array;       // does the user want an array of the palette?
    bool output_palette_appvar;      // does the user want the palette inside the appvar?
    uint8_t bpp;                     // bits per pixel in each image
    
    // for creating global palettes
    bool is_global_palette;          // should we just build a palette rather than a group?
} group_t;

typedef struct c_st {
    FILE *ini;
    FILE *log;
    FILE *all_gfx_c;
    FILE *all_gfx_h;
    unsigned numgroups;
    unsigned numappvars;
    unsigned curline;
    bool bad_conversion;
    bool icon_zds;
    char *iconc;
    bool using_custom_ini;
    bool using_custom_log;
    bool group_mode;
    char *directory;
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
    void (*print_image_header)(output_t *out, const char *i_name, unsigned int size, bool compressed);
    void (*print_transparent_image_header)(output_t *out, const char *i_name, unsigned int size, bool compressed);
    void (*print_palette_header)(output_t *out, const char *name, unsigned int len);
    void (*print_end_header)(output_t *out);
    
    // appvar functions
    void (*print_appvar_array)(output_t *out, const char *a_name, unsigned int num_images);
    void (*print_appvar_image)(output_t *out, const char *a_name, unsigned int offset, const char *i_name, unsigned int index, bool compressed, bool tp_style);
    void (*print_appvar_palette)(output_t *out, unsigned int offset);
    void (*print_appvar_load_function_header)(output_t *out);
    void (*print_appvar_load_function)(output_t *out, const char *a_name, bool has_tilemaps);
    void (*print_appvar_load_function_tilemap)(output_t *out, const char *a_name, char *tilemap_name, unsigned int tilemap_size, unsigned int index, bool compressed);
    void (*print_appvar_load_function_end)(output_t *out);
    void (*print_appvar_palette_header)(output_t *out, const char *p_name, const char *a_name, unsigned int index, unsigned int len);
} format_t;

extern convpng_t convpng;
extern group_t group[NUM_GROUPS];

#endif

