#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "libimagequant.h"
#include "lodepng.h"
#include "rle.h"
#include "lz.h"

// version information
#define VERSION_MAJOR 4
#define VERSION_MINOR 0

// prototypes
void errorf(char *format, ...);
void lof(char *format, ...);
char *get_line(FILE *stream);
int make_args(char *s, char ***args, const char *delim);
void free_args(char **args, char ***argv, unsigned argc);
void print_images(void);
void add_image(char *line);
int parse_input(void);
void add_rgba(uint8_t *pal, size_t size);
int create_icon(void);
char *rle_encode(char *str);
inline uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8);

// default names for ini and log files
const char ini_file_name[] = "convpng.ini";
const char log_file_name[] = "convpng.log";

#define MODE_C       0               // group is C
#define MODE_ASM     1               // group is asm
#define CMP_NONE     0               // no compression
#define CMP_RLE      1               // rle compression
#define CMP_LZ       3               // lz77 compression
#define NUM_GROUPS   256             // total number of groups able to create

typedef struct s_st {
    char *in;                        // name of image on disk
    char *outc;                      // output file name (.c, .asm)
    char *name;                      // name of image
    uint8_t *rgba;                   // pointer to large rgba array for quantization
    uint8_t *data;                   // pointer to image data
    uint8_t *pal;                    // pointer to image palette
    unsigned width,height;           // width of image, height of image
    size_t size;                     // total size of the image
} image_t;

typedef struct g_st {
    char *name;                      // name of the group file
    char *palette_name;              // custom palette file name
    unsigned palette_length;         // custom palette length
    image_t **image;                 // pointer to array of images
    int numimages;                   // number of images in the group
    char *outh;                      // output main .inc or .h file
    char *outc;                      // output main .asm or .c file
    int tindex;                      // index to use as the transparent color
    uint16_t tcolor_converted;       // converted transparent color
    liq_color tcolor;                // apparently more about the transparent color
    int mode;                        // either asm or c style conversion
    int compression;                 // compression type
    unsigned tile_width,tile_height; // for use with creating tilemaps
    unsigned tile_size;              // tile_height * tile_width + 2
    unsigned total_tiles;            // number of tiles in the image
    bool convert_to_tilemap;         // should we convert to a tilemap?
    bool create_tilemap_ptrs;        // should we create an array of pointers to the tiles?
    bool output_palette_image;       // does the user want an image of the palette?
    bool output_palette_array;       // does the user want an array of the palette?
    
    // for creating global palettes
    bool is_global_palette;          // should we just build a palette rather than a group?
} group_t;

typedef struct c_st {
    int numgroups;
    FILE *ini;
    FILE *log;
    char *line;
    char **argv;
    int curline;
    FILE *all_gfx_c;
    FILE *all_gfx_h;
    uint8_t *all_rgba;
    int all_rgba_size;
    char *iconc;
    bool bad_conversion;
    bool icon_zds;
} convpng_t;

convpng_t convpng;
group_t group[NUM_GROUPS];

extern uint8_t xlibc_palette[];
extern uint8_t rgb332_palette[];

char buffer[512];
void errorf(char *format, ...) {
   *buffer = 0;
   va_list aptr;

   va_start(aptr, format);

   vsprintf(buffer, format, aptr);
   if(convpng.log) { fprintf(convpng.log, "[error line %d] %s", convpng.curline, buffer); }
   fprintf(stderr, "[error line %d] %s", convpng.curline, buffer);
   
   va_end(aptr);
   exit(1);
}

void lof(char *format, ...) {
   *buffer = 0;
   va_list aptr;

   va_start(aptr, format);
   
   vsprintf(buffer, format, aptr);
   if(convpng.log) { fprintf(convpng.log, "%s", buffer); }
   fprintf(stdout, "%s", buffer);
   
   va_end(aptr);
}

inline uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8) {
    uint8_t r5 = round((int)r8 * 31 / 255.0);
    uint8_t g6 = round((int)g8 * 63 / 255.0);
    uint8_t b5 = round((int)b8 * 31 / 255.0);
    return ((g6 & 1) << 15) | (r5 << 10) | ((g6 >> 1) << 5) | b5;
}

char *str_dup(const char *s) {
    char *d = malloc (strlen (s) + 1);   // Allocate memory
    if (d != NULL) strcpy (d,s);         // Copy string if okay
    return d;                            // Return new memory
}

// encodes a PNG image (used for creating global palettes)
void encodePNG(const char* filename, const unsigned char* image, unsigned width, unsigned height) {
    unsigned char* png;
    size_t pngsize;

    unsigned error = lodepng_encode32(&png, &pngsize, image, width, height);
    if(!error) { lodepng_save_file(png, pngsize, filename); }
    
    /* if there's an error, display it */
    if(error) { printf("error %u: %s\n", error, lodepng_error_text(error)); }

    free(png);
}

// builds an image of the palette
void build_image_palette(const liq_palette *pal, const unsigned length, const char *filename) {
    unsigned char* image = (unsigned char*)malloc(length * 4);
    unsigned x;
    for(x = 0; x < length; x++) {
        image[4 * x + 0] = pal->entries[x].r;
        image[4 * x + 1] = pal->entries[x].g;
        image[4 * x + 2] = pal->entries[x].b;
        image[4 * x + 3] = 255;
    }
    encodePNG(filename, image, length, 1);
    free(image);
    lof("Saved palette (%s)\n",filename);
}

int main(int argc, char **argv) {
    char opt;
    char *ini = NULL;
    char *ext = NULL;
    time_t c1 = time(NULL);
    convpng.iconc = NULL;
    convpng.bad_conversion = false;
    
    while ( (opt = getopt(argc, argv, "c:i:j:") ) != -1) {
        switch (opt) {
            case 'c':	// generate an icon header file useable with the C toolchain
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = true;
                return create_icon();
            case 'j':	// generate an icon for asm programs
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = false;
                return create_icon();
            case 'i':	// change the ini file input
                ini = (char*)malloc(sizeof(char)*(strlen(optarg)+5));
                strcpy(ini, optarg);
                ext = strrchr(ini,'.');
                if(ext == NULL) {
                    strcat(ini,".ini");
                }
                break;
            default:
                break;
        }
    }

    lof("ConvPNG %d.%d by M.Waltz\n\n", VERSION_MAJOR, VERSION_MINOR);
    
    int s,g;
    unsigned j,k;
    const char *ini_file = ini ? ini : ini_file_name;
    const char *log_file = log_file_name;
    FILE *outc;

    convpng.ini = fopen(ini_file,"r");
    convpng.log = fopen(log_file,"w");
    convpng.curline = 0;
    convpng.all_rgba_size = 0;
    convpng.numgroups = 0;
    convpng.all_rgba = NULL;
    for(s = 0; s < NUM_GROUPS; s++) {
        group[s].image = NULL;
        group[s].name = NULL;
        group[s].tindex = -1;
        group[s].numimages = 0;
        group[s].mode = 0;
    }
    
    if(!convpng.ini) { lof("[error] could not find file '%s'.\nPlease make sure you have created the configuration file\n",ini_file);  exit(1); }
    if(!convpng.log) { lof("could not open file '%s'.\nPlease check file permissions\n",log_file); exit(1); }
    
    lof("Opened %s\n", ini_file);
    
    while ((convpng.line = get_line(convpng.ini))) {
        int num = parse_input();
        free_args(&convpng.line, &convpng.argv, num);
    }
    fclose(convpng.ini);

    lof("\n");
    
    // convert all the groups
    for(g = 0; g < convpng.numgroups; g++) {
        double diff;
        liq_palette custom_pal;
        liq_image *image = NULL;
        liq_result *res = NULL;
        liq_attr *attr = NULL;
        liq_palette *pal = NULL;
        
        if(group[g].is_global_palette) {
            lof("--- Global Palette %s ---\n",group[g].name);
        } else {
            lof("--- Group %s (%s) ---\n",group[g].name,group[g].mode == MODE_ASM ? "ASM" : "C");
            convpng.all_gfx_c = fopen(group[g].outc,"w");
            convpng.all_gfx_h = fopen(group[g].outh,"w");
            if(!convpng.all_gfx_c || !convpng.all_gfx_h) { errorf("could not open %s for output.", group[g].name); }
        }
        
        attr = liq_attr_create();
        if(!attr) { errorf("could not create image attributes."); }
        
        group[g].palette_length = 256;
        
        /* build the palette */
        liq_set_max_colors(attr, group[g].palette_length);
        
        /* check if we need a custom palette */
        if(group[g].palette_name != NULL) {
            if(!strcmp(group[g].palette_name, "xlibc")) {
                unsigned h;
                liq_color rgba_color;
                
                for(h = 0; h < 256; h++) {
                    rgba_color.r = xlibc_palette[(h * 3) + 0];
                    rgba_color.g = xlibc_palette[(h * 3) + 1];
                    rgba_color.b = xlibc_palette[(h * 3) + 2];
                    rgba_color.a = 0xFF;
                    
                    custom_pal.entries[h] = rgba_color;
                }
                lof("Using built-in xlibc palette...\n");
            } else if(!strcmp(group[g].palette_name, "rgb332")) {
                unsigned h;
                liq_color rgba_color;
                
                for(h = 0; h < 256; h++) {
                    rgba_color.r = rgb332_palette[(h * 3) + 0];
                    rgba_color.g = rgb332_palette[(h * 3) + 1];
                    rgba_color.b = rgb332_palette[(h * 3) + 2];
                    rgba_color.a = 0xFF;
                    
                    custom_pal.entries[h] = rgba_color;
                }
                lof("Using built-in rgb332 palette...\n");
           } else {
                // some variables
                uint8_t *rgba;
                unsigned width, height, size;
                
                // decode the palette
                unsigned error = lodepng_decode32_file(&rgba, &width, &height, group[g].palette_name);
                if(error) { errorf("%s: %s", lodepng_error_text(error), group[g].palette_name); }
                if(height > 1 || width > 256 || width < 3) { errorf("palette not formatted correctly."); }
                
                size = (width * height) << 2;
                add_rgba(rgba, size);
                
                // free the opened image
                free(rgba);
                group[g].palette_length = width;
                lof("Using defined palette %s...\n", group[g].palette_name);
                
                unsigned h;
                liq_color rgba_color;
                custom_pal.count = group[g].palette_length;
                
                for(h = 0; h < group[g].palette_length; h++) {
                    rgba_color.r = convpng.all_rgba[(h * 4) + 0];
                    rgba_color.g = convpng.all_rgba[(h * 4) + 1];
                    rgba_color.b = convpng.all_rgba[(h * 4) + 2];
                    rgba_color.a = convpng.all_rgba[(h * 4) + 3];
                    
                    custom_pal.entries[h] = rgba_color;
                }
            }
            pal = &custom_pal;
        } else {
            lof("Building Palette...\n");
            for(s = 0; s < group[g].numimages; s++) {
                unsigned error;
		    
                // open the file
                error = lodepng_decode32_file(&group[g].image[s]->rgba, &group[g].image[s]->width, &group[g].image[s]->height, group[g].image[s]->in);
                if(error) { errorf("%s: %s", lodepng_error_text(error),group[g].image[s]->in); }
                
                group[g].image[s]->size = group[g].image[s]->width*group[g].image[s]->height;
                add_rgba(group[g].image[s]->rgba, group[g].image[s]->size << 2);
                
                // free the opened image
                free(group[g].image[s]->rgba);
            }
            
            image = liq_image_create_rgba(attr, convpng.all_rgba, convpng.all_rgba_size/4, 1, 0);
            if(group[g].tindex >= 0) {
                liq_image_add_fixed_color(image,group[g].tcolor);
            }
            if(!image) { errorf("could not create palette."); }
            res = liq_quantize_image(attr, image);
            if(!res) {errorf("could not quantize palette."); }
            pal = (liq_palette*)liq_get_palette(res);
            diff = liq_get_quantization_error(res);
            if(diff > 12) {
                convpng.bad_conversion = true;
            }
            
            lof("Palette quality : %.2f%%\n",100-diff);
        }
        
        // find the transparent color
        if(group[g].tindex >= 0) {
            for (j = 0; j < group[g].palette_length ; j++) {
                if(group[g].tcolor_converted == rgb1555(pal->entries[j].r,pal->entries[j].g,pal->entries[j].b)) {
                    group[g].tindex = j;
                }
            }
	    // move transparent color to index 0
	    liq_color tmpp = pal->entries[group[g].tindex];
            pal->entries[group[g].tindex] = pal->entries[0];
            pal->entries[0] = tmpp;
        }
        
        // get the number of entires
        unsigned count = (unsigned)(group[g].tindex == -1) ? group[g].palette_length : (unsigned)(group[g].tindex + 1);
        
        // output an image of the palette
        if (group[g].output_palette_image) {
            char *png_file = (char*)malloc(sizeof(char)*(strlen(group[g].name)+10));
            strcpy(png_file, group[g].name);
            strcat(png_file, "_pal.png");
            build_image_palette(pal, group[g].palette_length, png_file);
            free(png_file);
        }
        
        // check and see if we need to build a global palette
        if(group[g].is_global_palette) {
            // build an image which uses the global palette
            build_image_palette(pal, group[g].palette_length, group[g].name);
        } else {
            // now, write the all_gfx output
            if(group[g].mode == MODE_C) {
                fprintf(convpng.all_gfx_c, "// Converted using ConvPNG\n");
                fprintf(convpng.all_gfx_c, "#include <stdint.h>\n");
                fprintf(convpng.all_gfx_c, "#include \"%s\"\n\n",group[g].outh);
                
                if (group[g].output_palette_array) {
                    fprintf(convpng.all_gfx_c, "uint16_t %s_pal[%u] = {\n",group[g].name,count);
            
                    for(j = 0; j < count; j++) {
                        fprintf(convpng.all_gfx_c, " 0x%04X%c  // %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(pal->entries[j].r,pal->entries[j].g,pal->entries[j].b),j==255 ? ' ' : ',', j,pal->entries[j].r,pal->entries[j].g,pal->entries[j].b,pal->entries[j].a);
                    }
                    fprintf(convpng.all_gfx_c, "};");
                }
                
                fprintf(convpng.all_gfx_h, "// Converted using ConvPNG\n");
                fprintf(convpng.all_gfx_h, "// This file contains all the graphics sources for easier inclusion in a project\n");
                fprintf(convpng.all_gfx_h, "#ifndef %s_H\n#define %s_H\n",group[g].name,group[g].name);
                fprintf(convpng.all_gfx_h, "#include <stdint.h>\n\n");
            } else {
                fprintf(convpng.all_gfx_c, "; Converted using ConvPNG\n");
                fprintf(convpng.all_gfx_c, "; This file contains all the graphics for easier inclusion in a project\n\n");
                
                if (group[g].output_palette_array) {
                    fprintf(convpng.all_gfx_c, "_%s_pal_size equ %u\n",group[g].name,count<<1);
                    fprintf(convpng.all_gfx_c, "_%s_pal:\n",group[g].name);
                
                    for(j = 1; j < count; j++) {
                        fprintf(convpng.all_gfx_c, " dw 0%04Xh ; %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(pal->entries[j].r,pal->entries[j].g,pal->entries[j].b),j,pal->entries[j].r,pal->entries[j].g,pal->entries[j].b,pal->entries[j].a);
                    }
                }
                
                fprintf(convpng.all_gfx_h, "; Converted using ConvPNG\n");
                fprintf(convpng.all_gfx_h, "; This file contains all the graphics for easier inclusion in a project\n");
                fprintf(convpng.all_gfx_h, "#ifndef %s_H\n#define %s_H\n\n",group[g].name,group[g].name);
                fprintf(convpng.all_gfx_h, "; ZDS sillyness\n#define db .db\n#define dw .dw\n#define dl .dl\n\n");
                if(group[g].tindex >= 0) {
                    fprintf(convpng.all_gfx_h, "%s_transpcolor_index equ %d\n\n",group[g].name,0);
                }
                fprintf(convpng.all_gfx_h, "#include \"%s\"\n",group[g].outc);
            }
            
            if(group[g].tindex >= 0) {
                lof("TranspColorIndex : 0x00\n");
                lof("TranspColor : 0x%04X\n",group[g].tcolor_converted);
            }

            lof("%d:\n",group[g].numimages);
            
            // okay, now we have the palette used for all the images. Let's fix them to the attributes
            for(s = 0; s < group[g].numimages; s++) {
                unsigned error;
                liq_image *sp_image = NULL;
                liq_result *sp_res = NULL;
                liq_attr *sp_attr = liq_attr_create();
                
                // open the file
                error = lodepng_decode32_file(&group[g].image[s]->rgba, &group[g].image[s]->width, &group[g].image[s]->height, group[g].image[s]->in);
                if(error) { errorf("%s: %s", lodepng_error_text(error),group[g].image[s]->in); }
                
                group[g].image[s]->size = group[g].image[s]->width * group[g].image[s]->height;
                
                if(group[g].convert_to_tilemap) {
                    group[g].total_tiles = (group[g].image[s]->width / group[g].tile_width) * (group[g].image[s]->height / group[g].tile_height);
                }
                
                if(group[g].convert_to_tilemap == true) {
                    if((group[g].image[s]->width % group[g].tile_width) || (group[g].image[s]->height % group[g].tile_height)) {
                        errorf("image dimensions do not match tile width and/or height values.");
                    }
                }
                
                group[g].image[s]->data = (uint8_t*)malloc(group[g].image[s]->size + 1);
                
                liq_set_max_colors(sp_attr, group[g].palette_length);
                sp_image = liq_image_create_rgba(sp_attr, group[g].image[s]->rgba, group[g].image[s]->width, group[g].image[s]->height, 0);
                if(!sp_image) { errorf("could not create image."); }
                for(j = 0; j < group[g].palette_length; j++) {
                    liq_image_add_fixed_color(sp_image,pal->entries[j]);
                }
                sp_res = liq_quantize_image(attr, sp_image);
                if(!sp_res) {errorf("could not quantize image."); }
                
                liq_write_remapped_image(sp_res, sp_image, group[g].image[s]->data, group[g].image[s]->size);
                
                if(group[g].palette_name == NULL) {
                    diff = liq_get_remapping_error(sp_res);
                    if(diff > 12) {
                        convpng.bad_conversion = true;
                    }
                    lof(" %s : %.2f%%",group[g].image[s]->name,100-diff);
                } else {
                    lof(" %s : Converted!",group[g].image[s]->name);
                    convpng.bad_conversion = true;
                }
                
                // open the outputs
                if(!(outc = fopen(group[g].image[s]->outc,"w"))) { 
                    errorf("opening file for output.");
                }
                
                if(group[g].mode == MODE_C) {
                    fprintf(outc,"// Converted using ConvPNG\n");
                    fprintf(outc,"#include <stdint.h>\n#include \"%s\"\n\n",group[g].outh);
                } else {
                    fprintf(outc,"; Converted using ConvPNG\n\n");
                }
                
                if(group[g].mode == MODE_ASM) {
                    if(group[g].convert_to_tilemap == false) {
                            fprintf(outc,"_%s_width equ %u\n",group[g].image[s]->name,group[g].image[s]->width);
                            fprintf(outc,"_%s_height equ %u\n",group[g].image[s]->name,group[g].image[s]->height);
                    }
                }
                
                if(group[g].compression != CMP_NONE) {
                    unsigned compressed_size;
                    
                    if(group[g].convert_to_tilemap == false) {
                        uint8_t *tmp_data = (uint8_t*)malloc(group[g].image[s]->size + 3);   
                        tmp_data[0] = group[g].image[s]->width;
                        tmp_data[1] = group[g].image[s]->height;
                    
                        memcpy(&tmp_data[2], group[g].image[s]->data, group[g].image[s]->size);
                        uint8_t *data = (uint8_t*)malloc((group[g].image[s]->size * 2) + 4);
                        switch(group[g].compression) {
                            case CMP_LZ:
                                compressed_size = (unsigned)LZ_Compress(tmp_data,data,group[g].image[s]->size);
                                break;
                            default:
                                compressed_size = (unsigned)RLE_Compress(tmp_data,data,group[g].image[s]->size);
                                break;
                        }
                        if(group[g].mode == MODE_C) {
                            fprintf(outc,"uint8_t %s_data_compressed[%u] = {\n 0x%02X,0x%02X,\n ",group[g].image[s]->name,compressed_size + 2,compressed_size & 0xFF,(compressed_size >> 8) & 0xFF);
                        } else {
                            fprintf(outc,"_%s_data_compressed_size equ %u\n",group[g].image[s]->name,compressed_size + 2);
                            fprintf(outc,"_%s_data_compressed:\n db ",group[g].image[s]->name);
                        }
			
                        unsigned y = 0;
			
                        for(j = 0; j < compressed_size; j++) {
                            if(y++ == 20) {
                                y = 0;
                                fputc('\n',outc);
                            }
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"0x%02X,",data[j]);
                            } else {
                                fprintf(outc,"0%02Xh%c",data[j],j+1 == compressed_size ? ' ' : ',');
                            }
                        }
                        if(group[g].mode == MODE_C) {
                            fprintf(outc,"\n};\n");
                        }
                        lof(" (compression: %u -> %d bytes) (%s)\n",group[g].image[s]->size,compressed_size + 2,group[g].image[s]->outc);
                        group[g].image[s]->size = compressed_size + 2;
                        free(tmp_data);
                        free(data);
                    } else {
                        unsigned curr_tile, q;
                        unsigned x_offset, y_offset, offset;
                        uint8_t *data = (uint8_t*)malloc(group[g].tile_size * 2);
                        uint8_t *tmp_data = (uint8_t*)malloc(group[g].tile_size + 3);
                        
                        x_offset = y_offset = 0;
                        
                        for(curr_tile = 0; curr_tile < group[g].total_tiles; curr_tile++) {
                            tmp_data[0] = group[g].tile_width;
                            tmp_data[1] = group[g].tile_height;
                            q = 2;
                            
                            // convert a single tile
                            for(j = 0; j < group[g].tile_height; j++) {
                                offset = j * group[g].image[s]->width;
                                for(k = 0; k < group[g].tile_width; k++) {
                                    tmp_data[q++] = group[g].image[s]->data[k + x_offset + y_offset + offset];
                                }
                            }
                            
                            switch(group[g].compression) {
                                case CMP_LZ:
                                    compressed_size = (unsigned)LZ_Compress(tmp_data,data,group[g].tile_size);
                                    break;
                                case CMP_RLE:
                                    compressed_size = (unsigned)RLE_Compress(tmp_data,data,group[g].tile_size);
                                    break;
                                default:
                                    errorf("unexpected compression mode.");
                                    break;
                            }
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"uint8_t %s_tile_%u_data_compressed[%u] = {\n 0x%02X,0x%02X,\n ",group[g].image[s]->name,curr_tile,compressed_size + 2,(compressed_size+2) & 0xFF,((compressed_size+2) >> 8) & 0xFF);
                            } else {
                                fprintf(outc,"_%s_tile_%u_size equ %u\n",group[g].image[s]->name,curr_tile,compressed_size + 2);
                                fprintf(outc,"_%s_tile_%u_compressed:\n db ",group[g].image[s]->name,curr_tile);
                            }
                            
                            unsigned y = 0;
                            
                            for(j = 0; j < compressed_size; j++) {
                                if(y++ == 20) {
                                    y = 0;
                                    fputc('\n',outc);
                                }
                                if(group[g].mode == MODE_C) {
                                    fprintf(outc,"0x%02X,",data[j]);
                                } else {
                                    fprintf(outc,"%02X%c",data[j],j+1 == compressed_size ? ' ' : ',');
                                }
                            }
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"\n};\n");
                            }
                compressed_size += 2;
                            lof("\n %s_tile_%u_compressed (compression: %u -> %d bytes) (%s)",group[g].image[s]->name,curr_tile,group[g].tile_size,compressed_size,group[g].image[s]->outc);
                            group[g].image[s]->size = compressed_size;
                            
                            // move to the correct data location
                            if((x_offset += group[g].tile_width) > group[g].image[s]->width - 1) {
                                x_offset = 0;
                                y_offset += group[g].image[s]->width * group[g].tile_height;
                            }
                        }
                        
                        
                        // build the tilemap table
                        if(group[g].create_tilemap_ptrs) {
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"uint8_t *%s_tiles_data_compressed[%u] = {\n",group[g].image[s]->name, group[g].total_tiles);
                                for(curr_tile = 0; curr_tile < group[g].total_tiles; curr_tile++) {
                                    fprintf(outc," %s_tile_%u_data_compressed,\n",group[g].image[s]->name, curr_tile);
                                }
                                fprintf(outc,"};\n");
                            } else {
                                fprintf(outc,"_%s_tiles_compressed: ; %u tiles\n",group[g].image[s]->name, group[g].total_tiles);
                                for(curr_tile = 0; curr_tile < group[g].total_tiles; curr_tile++) {
                                    fprintf(outc," dl _%s_tile_%u_compressed\n",group[g].image[s]->name, curr_tile);
                                }
                            }
                        }
                        
                        free(tmp_data);
                        free(data);
                    }
                } else {
                    unsigned curr_tile;
                    unsigned x_offset, y_offset, offset;
                    
                    x_offset = y_offset = 0;
                    
                    lof(" (%s)\n",group[g].image[s]->outc);
                    
                    if(group[g].convert_to_tilemap == true) {
                        // convert the file with the tilemap formatting
                        for(curr_tile = 0; curr_tile < group[g].total_tiles; curr_tile++) {
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"uint8_t %s_tile_%u_data[%u] = {\n %u,\t// tile_width\n %u,\t// tile_height\n ", group[g].image[s]->name,
                                                                                                                              curr_tile,
                                                                                                                              group[g].tile_size,
                                                                                                                              group[g].tile_width,
                                                                                                                              group[g].tile_height);
                            } else {
                                fprintf(outc,"_%s_tile_%u: ; %u bytes\n db %u,%u ; width,height\n db ", group[g].image[s]->name,
                                                                                                        curr_tile,
                                                                                                        group[g].tile_size,
                                                                                                        group[g].tile_width,
                                                                                                        group[g].tile_height);
                            }
                            
                            // convert a single tile
                            for(j = 0; j < group[g].tile_height; j++) {
                                offset = j * group[g].image[s]->width;
                                for(k = 0; k < group[g].tile_width; k++) {
                                    if(group[g].mode == MODE_C) {
                                        fprintf(outc,"0x%02X%c",group[g].image[s]->data[k + x_offset + y_offset + offset],',');
                                    } else {
                                        fprintf(outc,"0%02Xh%c", group[g].image[s]->data[k + x_offset + y_offset + offset],
                                                k+1 == group[g].tile_width ? ' ' : ',');
                                    }
                                }
                                // check if at the end
                                if(group[g].mode == MODE_C) {
                                    fprintf(outc,"\n%s",j+1 != group[g].tile_height ? " " : "};\n\n");
                                } else {
                                    fprintf(outc,"\n%s",j+1 != group[g].tile_height ? " db " : "\n\n");
                                }
                            }
                            
                            // move to the correct data location
                            if((x_offset += group[g].tile_width) > group[g].image[s]->width - 1) {
                                x_offset = 0;
                                y_offset += group[g].image[s]->width * group[g].tile_height;
                            }
                        }
                        
                        // build the tilemap table
                        if(group[g].create_tilemap_ptrs) {
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"uint8_t *%s_tiles_data[%u] = {\n",group[g].image[s]->name, group[g].total_tiles);
                                for(curr_tile = 0; curr_tile < group[g].total_tiles; curr_tile++) {
                                    fprintf(outc," %s_tile_%u_data,\n",group[g].image[s]->name, curr_tile);
                                }
                                fprintf(outc,"};\n");
                            } else {
                                fprintf(outc,"_%s_tiles: ; %u tiles\n",group[g].image[s]->name, group[g].total_tiles);
                                for(curr_tile = 0; curr_tile < group[g].total_tiles; curr_tile++) {
                                    fprintf(outc," dl _%s_tile_%u\n",group[g].image[s]->name, curr_tile);
                                }
                            }
                        }
                    } else {
                        // convert the file with the standard formatting
                        if(group[g].mode == MODE_C) {
                            fprintf(outc,"uint8_t %s_data[%u] = {\n %u,\t/* width */\n %u,\t/* height */\n ",group[g].image[s]->name,group[g].image[s]->size+2,group[g].image[s]->width,group[g].image[s]->height);
                        } else {
                            fprintf(outc,"_%s: ; %u bytes\n db ",group[g].image[s]->name, group[g].image[s]->size + 2);
                        }
                        for(j = 0; j < group[g].image[s]->height ; j++) {
                            int offset = j*group[g].image[s]->width;
                            for(k = 0; k < group[g].image[s]->width ; k++) {
                                if(group[g].mode == MODE_C) {
                                    fprintf(outc,"0x%02X%c",group[g].image[s]->data[k+offset],',');
                                } else {
                                    char sp = k+1 == group[g].image[s]->width ? ' ' : ',';
                                    fprintf(outc,"0%02Xh%c",group[g].image[s]->data[k+offset],sp);
                                }
                            }
                            if(group[g].mode == MODE_C) {
                                fprintf(outc,"\n%s",j+1 != group[g].image[s]->height ? " " : "};\n");
                            } else {
                                fprintf(outc,"\n%s",j+1 != group[g].image[s]->height ? " db " : "\n");
                            }
                        }
                        
                    }
                }
                
                if(group[g].convert_to_tilemap == true) {
                    unsigned curr_tile = 0;
                    if(group[g].mode == MODE_C) {
                        for(; curr_tile < group[g].total_tiles; curr_tile++) {
                            if(group[g].compression == CMP_NONE) {
                                fprintf(convpng.all_gfx_h,"extern uint8_t %s_tile_%u_data[%u];\n",group[g].image[s]->name, curr_tile, group[g].tile_size);
                                fprintf(convpng.all_gfx_h, "#define %s_tile_%u ((gfx_image_t*)%s_tile_%u_data)\n",group[g].image[s]->name,curr_tile,group[g].image[s]->name,curr_tile);
                            } else {
                                fprintf(convpng.all_gfx_h,"extern uint8_t %s_tile_%u_data_compressed[];\n",group[g].image[s]->name, curr_tile);
                            }
                        }

                        if(group[g].create_tilemap_ptrs) {
                            if(group[g].compression == CMP_NONE) {
                                fprintf(convpng.all_gfx_h, "extern uint8_t *%s_tiles_data[%u];\n",group[g].image[s]->name,group[g].total_tiles);
                                fprintf(convpng.all_gfx_h, "#define %s_tiles ((gfx_image_t**)%s_tiles_data)\n",group[g].image[s]->name,group[g].image[s]->name);
                            } else {
                                fprintf(convpng.all_gfx_h, "extern uint8_t *%s_tiles_data_compressed[%u];\n",group[g].image[s]->name,group[g].total_tiles);
                            }
                        }
                    } else {
                        fprintf(convpng.all_gfx_h, "#include \"%s\"\n",group[g].image[s]->outc);
                    }
                } else {
                    if(group[g].mode == MODE_C) {
                        if(group[g].compression == CMP_NONE) {
                            fprintf(convpng.all_gfx_h, "extern uint8_t %s_data[%u];\n",group[g].image[s]->name,group[g].image[s]->size + 2);
                            fprintf(convpng.all_gfx_h, "#define %s ((gfx_image_t*)%s_data)\n",group[g].image[s]->name,group[g].image[s]->name);
                        } else {
                            fprintf(convpng.all_gfx_h, "extern uint8_t %s_data_compressed[%u];\n",group[g].image[s]->name,group[g].image[s]->size);
                        }
                    } else {
                        fprintf(convpng.all_gfx_h, "#include \"%s\" ; %u bytes\n",group[g].image[s]->outc, group[g].image[s]->size);
                    }
                }
                
                // close the outputs
                fclose(outc);
                
                // free the opened image
                free(group[g].image[s]->rgba);
                free(group[g].image[s]->data);
                liq_attr_destroy(sp_attr);
                liq_result_destroy(sp_res);
                liq_image_destroy(sp_image);
            }
        
            if (group[g].output_palette_array) {
                if (group[g].mode == MODE_C) {
                    fprintf(convpng.all_gfx_h, "extern uint16_t %s_pal[%u];\n",group[g].name,count);
                }
            }
            fprintf(convpng.all_gfx_h, "\n#endif\n");
            fclose(convpng.all_gfx_h);
            fclose(convpng.all_gfx_c);
        }
        
        free(convpng.all_rgba);
        free(group[g].name);
        free(group[g].outc);
        free(group[g].outh);
        free(group[g].palette_name);
        liq_attr_destroy(attr);
        liq_image_destroy(image);
        convpng.all_rgba_size = 0;
        convpng.all_rgba = NULL;
        lof("\n");
    }
    
    lof("Converted in %u s\n\n",(unsigned)(time(NULL)-c1));
    if(convpng.bad_conversion) {
        lof("[warning] image quality might be too low.\nPlease try grouping similar images, reducing image colors, \nor selecting a better palette if conversion is not ideal.\n\n");
    }
    free(ini);
    lof("Finished!\n");
    
    return 0;
}

/**
 * frees the allocated memory for later use
 */
void free_args(char **args, char ***argv, unsigned argc) {
    if(argv) {
	unsigned i;
        for(i=0; i<argc; i++) {
            free((*argv)[i]);
            (*argv)[i] = NULL;
        }
        free(*argv);
        *argv = NULL;
    }
    if(*args) {
        free(*args);
        *args = NULL;
    }
}

/**
 * receive string
 */
char *get_line(FILE *stream) {   
    char *line = (char*)malloc(sizeof(char));
    char *tmp;
    if(feof(stream)) {
        free(line);
        return NULL;
    }

    if(line) {
	int i = 0, c = EOF;
        while (c) {
            c = fgetc(stream);
            
            if(c == '\n' || c == EOF) {
                c = '\0';
            }
            
            if(c != ' ') {
                line[i++] = (char)c;
                tmp = realloc(line, sizeof(char)*(i+1));
                if(tmp == NULL) {
                    errorf("fatal error. this is bad; really bad.");
                } else {
                    line = tmp;
                }
            }
        }
        convpng.curline++;
    }
    
    return line;
}

/**
 * makes a character string pointer array
 */
int make_args(char *s, char ***args, const char *delim) {
    unsigned argc = 0;
    char *token = strtok(s, delim);
    
    *args = NULL;
    
    if(token == NULL) {
        return -1;
    }
    
    // while there is a space present
    while(token != NULL) {
        size_t strl = strlen(token);
        
        // allocate the memory then copy it in
        *args = realloc(*args, sizeof(char*) * (argc + 1));
        (*args)[argc] = (char*)malloc(sizeof(char ) * (strl + 1));
        memcpy((*args)[argc], token, strl+1);
        
        // increment the argument number
        argc++;

        // get the next token
        token = strtok(NULL, delim);
    }
    *args = realloc(*args, sizeof(char*) * (argc + 1));
    (*args)[argc] = NULL;
    return argc;
}

void add_image(char *line) {
    int g = convpng.numgroups - 1;
    int s = group[g].numimages;
    char *ext;
    size_t strl = strlen(line) + 1;
    
    if(!(group[g].image = realloc(group[g].image, sizeof(image_t*) * (s + 1))))         goto err;
    if(!(group[g].image[s] = (image_t*)malloc(sizeof(image_t))))                        goto err;
    if(!(group[g].image[s]->pal = (uint8_t*)malloc(256)))                               goto err;
    
    if(!(group[g].image[s]->in = (char*)malloc(sizeof(char)*(strl+5))))                 goto err;
    strcpy(group[g].image[s]->in, line);
    ext = strrchr(group[g].image[s]->in,'.');
    if(ext == NULL) {
        strcat(group[g].image[s]->in,".png");
        ext = strrchr(group[g].image[s]->in,'.');
    }
 
    if(!(group[g].image[s]->outc = (char*)malloc(sizeof(char)*(strlen(group[g].image[s]->in)+5))))  goto err;
    strcpy(group[g].image[s]->outc,group[g].image[s]->in);
    strcpy(group[g].image[s]->outc+(ext-group[g].image[s]->in),group[g].mode == MODE_C ? ".c" : ".asm");
    
    if(!(group[g].image[s]->name = (char*)malloc(sizeof(char)*(strlen(group[g].image[s]->in)+1))))  goto err;
    strcpy(group[g].image[s]->name, group[g].image[s]->in);
    group[g].image[s]->name[(int)(strrchr(group[g].image[s]->name,'.')-group[g].image[s]->name)] = '\0';
    
    group[g].numimages++;
    
    return;

err:
    errorf("internal memory allocation error. please contact developer.\n");
}

void print_images(void) {
    int i;
    
    for (i = 0; i < group[0].numimages; i++) {
        lof("  %d  %s\n", i+1, group[0].image[i]->in);
    }
}

int parse_input(void) {
    int num = 0;
    if(convpng.line[0] != '\0') {   
        if(convpng.line[0] == '#') {
            num = make_args(convpng.line, &convpng.argv, ":");
            
            // set the transparent color
            if(!strcmp(convpng.argv[0], "#TranspColor")) {
                int g = convpng.numgroups - 1;
                char **colors;
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = make_args(convpng.argv[1], &colors, ",");
                if(num < 4) { errorf("not enough colors."); }
                
                group[g].tcolor.r = (uint8_t)atoi(colors[0]);
                group[g].tcolor.g = (uint8_t)atoi(colors[1]);
                group[g].tcolor.b = (uint8_t)atoi(colors[2]);
                group[g].tcolor.a = (uint8_t)atoi(colors[3]);
                group[g].tcolor_converted = rgb1555(group[g].tcolor.r,group[g].tcolor.g,group[g].tcolor.b);
                group[g].tindex = 0;
                
                // free the allocated memory
                free_args(&convpng.argv[1], &colors, num);
            }
            
            if(!strcmp(convpng.argv[0], "#PNGImages")) {
            }
           
            if(!strcmp(convpng.argv[0], "#Compression")) {
                if(!strcmp(convpng.argv[1], "rle")) {
                    group[convpng.numgroups-1].compression = CMP_RLE;
                }
                if(!strcmp(convpng.argv[1], "lz77")) {
                    group[convpng.numgroups-1].compression = CMP_LZ;
                }
            }
            
            if(!strcmp(convpng.argv[0], "#CreateGlobalPalette")) {
                int g = convpng.numgroups;
                group[g].is_global_palette = true;
                group[g].palette_name = NULL;
                group[g].compression = CMP_NONE;
                group[g].convert_to_tilemap = false;
                group[convpng.numgroups].mode = MODE_C;
                group[g].output_palette_image = false;
                group[g].name = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+1));
                strcpy(group[g].name,convpng.argv[1]);
                convpng.numgroups++;
            }
	    
            if(!strcmp(convpng.argv[0], "#Palette")) {
                group[convpng.numgroups-1].palette_name = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+1));
                strcpy(group[convpng.numgroups-1].palette_name,convpng.argv[1]);
            }
            
            if(!strcmp(convpng.argv[0], "#OutputPaletteImage")) {
                group[convpng.numgroups-1].output_palette_image = true;
            }
            
            if(!strcmp(convpng.argv[0], "#NoPaletteArray")) {
                group[convpng.numgroups-1].output_palette_array = false;
            }
            
            if(!strcmp(convpng.argv[0], "#Tilemap")) {
                int g = convpng.numgroups - 1;
                char **tilemap_options;
		
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = make_args(convpng.argv[1], &tilemap_options, ",");
                if(num < 3) { errorf("not enough options specified (tile_width,tile_hieght)."); }
                
                group[g].tile_width = (unsigned)atoi(tilemap_options[0]);
                group[g].tile_height = (unsigned)atoi(tilemap_options[1]);
                group[g].create_tilemap_ptrs = !strcmp(tilemap_options[2], "true");
                group[g].tile_size = group[g].tile_width * group[g].tile_height + 2;
                
                /* Free the allocated memory */
                free_args(&convpng.argv[1], &tilemap_options, num);
                group[g].convert_to_tilemap = true;
            }
	    
            if(!strcmp(convpng.argv[0], "#GroupC")) {
                int g = convpng.numgroups;
                group[g].output_palette_image = false;
                group[g].is_global_palette = false;
                group[g].palette_name = NULL;
                group[g].compression = CMP_NONE;
                group[g].convert_to_tilemap = false;
                group[g].output_palette_array = true;
                group[g].name = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+1));
                strcpy(group[g].name,convpng.argv[1]);
                group[g].outh = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+3));
                strcpy(group[g].outh,convpng.argv[1]);
                strcat(group[g].outh,".h");
                group[g].outc = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+3));
                strcpy(group[g].outc,convpng.argv[1]);
                strcat(group[g].outc,".c");
                group[convpng.numgroups].mode = MODE_C;
                convpng.numgroups++;
            }
            
            if(!strcmp(convpng.argv[0], "#GroupASM")) {
                int g = convpng.numgroups;
                group[g].output_palette_array = true;
                group[g].output_palette_image = false;
                group[g].is_global_palette = false;
                group[g].palette_name = NULL;
                group[g].compression = CMP_NONE;
                group[g].convert_to_tilemap = false;
                group[g].name = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+1));
                strcpy(group[g].name,convpng.argv[1]);
                group[g].outh = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+5));
                strcpy(group[g].outh,convpng.argv[1]);
                strcat(group[g].outh,".inc");
                group[g].outc = (char*)malloc(sizeof(char)*(strlen(convpng.argv[1])+5));
                strcpy(group[g].outc,convpng.argv[1]);
                strcat(group[g].outc,".asm");
                group[convpng.numgroups].mode = MODE_ASM;
                convpng.numgroups++;
            }
	    
        } else {
            add_image(convpng.line);
        }
    }
    return num;
}

// add an rgba color to the palette
void add_rgba(uint8_t *pal, size_t size) {
    convpng.all_rgba = realloc(convpng.all_rgba, sizeof(uint8_t)*(convpng.all_rgba_size+size+1));
    memcpy(convpng.all_rgba+convpng.all_rgba_size,pal,size);
    convpng.all_rgba_size += size;
}

// create an icon for the C toolchain
int create_icon(void) {
    liq_palette custom_pal;
    liq_image *image = NULL;
    liq_result *res = NULL;
    liq_attr *attr = NULL;
    uint8_t *rgba;
    uint8_t *data;
    unsigned width,height,size,error,x,y,h,j;
    liq_color rgba_color;
    char **icon_options;
    
    int num = make_args(convpng.iconc, &icon_options, ",");
    if(num < 2) { errorf("not enough options."); }
    
    error = lodepng_decode32_file(&rgba, &width, &height, icon_options[0]);
    if(error) { lof("[error] could not open %s for conversion\n", icon_options[0]); exit(1); }
    if(width != 16 || height != 16) { errorf("icon image dimensions are not 16x16."); }
    
    attr = liq_attr_create();
    if(!attr) { errorf("could not create image attributes."); }
    image = liq_image_create_rgba(attr, rgba, 16, 16, 0);
    if(!image) { errorf("could not create icon."); }

    size = width * height;

    for(h = 0; h < 256; h++) {
        rgba_color.r = xlibc_palette[(h * 3) + 0];
        rgba_color.g = xlibc_palette[(h * 3) + 1];
        rgba_color.b = xlibc_palette[(h * 3) + 2];
        rgba_color.a = 0xFF;
        
        custom_pal.entries[h] = rgba_color;
    }
    for(j = 0; j < 256; j++) {
        liq_image_add_fixed_color(image,custom_pal.entries[j]);
    }
    
    data = (uint8_t*)malloc(size + 1);
    res = liq_quantize_image(attr, image);
    if(!res) {errorf("could not quantize icon."); }
    liq_write_remapped_image(res, image, data, size);

    FILE *out = fopen("iconc.asm","w");
    if (convpng.icon_zds) {
        fprintf(out," define .icon,space=ram\n segment .icon\n xdef __icon_begin\n xdef __icon_end\n xdef __program_description\n xdef __program_description_end\n");
    
        fprintf(out,"\n db 1\n db %u,%u\n__icon_begin:",width,height);
        for(y = 0; y < height; y++) {
            fputs("\n db ",out);
            for(x = 0; x < width; x++) {
                fprintf(out,"0%02Xh%s",data[x+(y*width)], x + 1 == width ? "" : ",");
            }
        }

        fprintf(out,"\n__icon_end:\n__program_description:\n");
        fprintf(out," db \"%s\",0\n__program_description_end:\n",icon_options[1]);
        lof("Converted icon '%s'\n",icon_options[0]);
    } else {
        fprintf(out,"__icon_begin:\n .db 1,%u,%u",width,height);
        for(y = 0; y < height; y++) {
            fputs("\n .db ",out);
            for(x = 0; x < width; x++) {
                fprintf(out,"0%02Xh%s",data[x+(y*width)], x + 1 == width ? "" : ",");
            }
        }

        fprintf(out,"\n__icon_end:\n__program_description:\n");
        fprintf(out," .db \"%s\",0\n__program_description_end:\n",icon_options[1]);
        lof("Converted icon '%s' -> 'iconc.asm'\n",icon_options[0]);
    }
    
    liq_attr_destroy(attr);
    liq_image_destroy(image);
    free_args(&convpng.iconc, &icon_options, num);
    free(data);
    free(rgba);
    fclose(out);
    return 0;
}
