#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>

#include "libs/libimagequant.h"
#include "libs/lodepng.h"

#define VERSION_MAJOR 5              // version information
#define VERSION_MINOR 6              // minor version

#define MAX_PAL_LEN   256 
#define NUM_GROUPS    256            // total number of groups able to create

// function prototypes
void output_compressed_array(FILE *outfile, uint8_t *compressed_data, unsigned len, unsigned mode);
void add_rgba(uint8_t *pal, size_t size);
void init_convpng_struct(void);
void free_rgba(void);

enum mode_t { MODE_C = 0, MODE_ASM, MODE_ICE };                  // group is c, asm, sprites v3.3
enum compression_t { COMPRESS_NONE = 0, COMPRESS_RLE, COMPRESS_ZX7 }; // no compression, rle compression, lz compression

typedef struct s_st {
    char *in;                        // name of image on disk
    char *outc;                      // output file name (.c, .asm)
    char *name;                      // name of image
} image_t;

typedef struct g_st {
    char *name;                      // name of the group file
    char *palette_name;              // custom palette file name
    unsigned palette_length;         // custom palette length
    image_t **image;                 // pointer to array of images
    unsigned numimages;              // number of images in the group
    char *outh;                      // output main .inc or .h file
    char *outc;                      // output main .asm or .c file
    int tindex;                      // index to use as the transparent color
    uint16_t tcolor_converted;       // converted transparent color
    liq_color tcolor;                // apparently more about the transparent color
    bool use_tcolor;                 // bool to tell if to compute transparent color
    bool use_tindex;                 // bool to use a new index
    unsigned mode;                   // either asm or c style conversion
    unsigned compression;            // compression type
    unsigned tile_width,tile_height; // for use creating tilemaps
    unsigned tile_size;              // tile_height * tile_width + 2
    unsigned total_tiles;            // number of tiles in the image
    bool convert_to_tilemap;         // should we convert to a tilemap?
    bool create_tilemap_ptrs;        // should we create an array of pointers to the tiles?
    bool output_palette_image;       // does the user want an image of the palette?
    bool output_palette_array;       // does the user want an array of the palette?
    uint8_t bpp;                     // bits per pixel in each image
    
    // for creating global palettes
    bool is_global_palette;          // should we just build a palette rather than a group?
} group_t;

typedef struct c_st {
    unsigned numgroups;
    FILE *ini;
    FILE *log;
    char **argv;
    unsigned curline;
    FILE *all_gfx_c;
    FILE *all_gfx_h;
    uint8_t *all_rgba;
    unsigned all_rgba_size;
    char *iconc;
    bool bad_conversion;
    bool icon_zds;
} convpng_t;

extern convpng_t convpng;
extern group_t group[NUM_GROUPS];

#endif