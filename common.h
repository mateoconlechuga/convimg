#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "libs/libimagequant.h"
#include "libs/lodepng.h"

typedef struct {
    uint8_t *data;                   // converted image data
    unsigned int total_size;         // converted total size
    unsigned int size[8912];         // converted size(s)
    unsigned int num_sizes;          // converted number of needed sizes
} data_t;

typedef struct {
    char *in;                        // name of image on disk
    char *outc;                      // output file name (.c, .asm)
    char *name;                      // name of image
    unsigned int compression;        // compression information
    unsigned int style;              // output style
    bool convert_to_tilemap;         // should we convert to a tilemap?
    bool create_tilemap_ptrs;        // should we create an array of pointers to the tiles?
    bool found;                      // if it is ready to use
    unsigned int numtiles;           // number of tiles in the image
    unsigned int width;              // save image width
    unsigned int height;             // save image height
    data_t block;                    // converted image data
} image_t;

typedef struct {
    liq_color color;
    unsigned int index;
    bool exact;
} fixed_t;

typedef struct {
    char *name;                      // name of the group file
    char *palette;                   // custom palette file name
    unsigned palette_length;         // custom palette length
    bool palette_fixed_length;       // bool to check if forced palette size
    image_t **image;                 // pointer to array of images
    unsigned int numimages;          // number of images in the group
    char *outh;                      // output main .inc or .h file
    char *outc;                      // output main .asm or .c file
    int tindex;                      // index to use as the transparent color
    liq_color tcolor;                // transparent color used for conversion
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
    fixed_t fixed[256];              // fixed colors to add
    unsigned int num_fixed_colors;   // number of fixed colors
    unsigned int oindex;             // index to use as the omit color
    liq_color ocolor;                // color to omit from output
    bool use_ocolor;                 // use the color ommission
    bool use_oindex;                 // use the index ommission
    bool output_size;                // output size bytes for images

    // for creating global palettes
    bool is_global_palette;          // should we just build a palette rather than a group?
} group_t;

#endif
