#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
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

#define VERSION_MAJOR 2
#define VERSION_MINOR 0

void errorf(char *format, ...);
void lof(char *format, ...);
char *get_line(FILE *stream);
int make_args(char *s, char ***args, const char *delim);
void free_args(char **args, char ***argv, unsigned argc);
void print_sprites(void);
void add_sprite(char *line);
int parse_input(void);
void add_rgba(uint8_t *pal, size_t size);
int create_icon(void);
char *rle_encode(char *str);
inline uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8);

const char ini_file_name[] = "convpng.ini";
const char log_file_name[] = "convpng.log";

#define MODE_C       0
#define MODE_ASM     1
#define CMP_NONE     0
#define CMP_RLE      1
#define CMP_LZ       3
#define NUM_GROUPS   256

typedef struct s_st {
    char *in;
    char *outc;
    char *name;
    uint8_t *rgba;
    uint8_t *data;
    uint8_t *pal;
    unsigned width,height;
    size_t size;
} sprite_t;

typedef struct g_st {
    char *name;
    sprite_t **sprite;
    int numsprites;
    char *outh;
    char *outc;
    liq_color tcolor;
    uint16_t tcolor_converted;
    int tindex;
    int mode;
    int compression;
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
} convpng_t;

convpng_t convpng;
group_t group[NUM_GROUPS];

/* xlibc color palette */
uint16_t hilo[] = {
 0x0000, 0x0081, 0x0102, 0x0183, 0x0204, 0x0285, 0x0306, 0x0387,
 0x0408, 0x0489, 0x050A, 0x058B, 0x060C, 0x068D, 0x070E, 0x078F,
 0x0810, 0x0891, 0x0912, 0x0993, 0x0A14, 0x0A95, 0x0B16, 0x0B97,
 0x0C18, 0x0C99, 0x0D1A, 0x0D9B, 0x0E1C, 0x0E9D, 0x0F1E, 0x0F9F,
 0x9000, 0x9081, 0x9102, 0x9183, 0x9204, 0x9285, 0x9306, 0x9387,
 0x9408, 0x9489, 0x950A, 0x958B, 0x960C, 0x968D, 0x970E, 0x978F,
 0x9810, 0x9891, 0x9912, 0x9993, 0x9A14, 0x9A95, 0x9B16, 0x9B97,
 0x9C18, 0x9C99, 0x9D1A, 0x9D9B, 0x9E1C, 0x9E9D, 0x9F1E, 0x9F9F,
 0x2020, 0x20A1, 0x2122, 0x21A3, 0x2224, 0x22A5, 0x2326, 0x23A7,
 0x2428, 0x24A9, 0x252A, 0x25AB, 0x262C, 0x26AD, 0x272E, 0x27AF,
 0x2830, 0x28B1, 0x2932, 0x29B3, 0x2A34, 0x2AB5, 0x2B36, 0x2BB7,
 0x2C38, 0x2CB9, 0x2D3A, 0x2DBB, 0x2E3C, 0x2EBD, 0x2F3E, 0x2FBF,
 0xB020, 0xB0A1, 0xB122, 0xB1A3, 0xB224, 0xB2A5, 0xB326, 0xB3A7,
 0xB428, 0xB4A9, 0xB52A, 0xB5AB, 0xB62C, 0xB6AD, 0xB72E, 0xB7AF,
 0xB830, 0xB8B1, 0xB932, 0xB9B3, 0xBA34, 0xBAB5, 0xBB36, 0xBBB7,
 0xBC38, 0xBCB9, 0xBD3A, 0xBDBB, 0xBE3C, 0xBEBD, 0xBF3E, 0xBFBF,
 0x4040, 0x40C1, 0x4142, 0x41C3, 0x4244, 0x42C5, 0x4346, 0x43C7,
 0x4448, 0x44C9, 0x454A, 0x45CB, 0x464C, 0x46CD, 0x474E, 0x47CF,
 0x4850, 0x48D1, 0x4952, 0x49D3, 0x4A54, 0x4AD5, 0x4B56, 0x4BD7,
 0x4C58, 0x4CD9, 0x4D5A, 0x4DDB, 0x4E5C, 0x4EDD, 0x4F5E, 0x4FDF,
 0xD040, 0xD0C1, 0xD142, 0xD1C3, 0xD244, 0xD2C5, 0xD346, 0xD3C7,
 0xD448, 0xD4C9, 0xD54A, 0xD5CB, 0xD64C, 0xD6CD, 0xD74E, 0xD7CF,
 0xD850, 0xD8D1, 0xD952, 0xD9D3, 0xDA54, 0xDAD5, 0xDB56, 0xDBD7,
 0xDC58, 0xDCD9, 0xDD5A, 0xDDDB, 0xDE5C, 0xDEDD, 0xDF5E, 0xDFDF,
 0x6060, 0x60E1, 0x6162, 0x61E3, 0x6264, 0x62E5, 0x6366, 0x63E7,
 0x6468, 0x64E9, 0x656A, 0x65EB, 0x666C, 0x66ED, 0x676E, 0x67EF,
 0x6870, 0x68F1, 0x6972, 0x69F3, 0x6A74, 0x6AF5, 0x6B76, 0x6BF7,
 0x6C78, 0x6CF9, 0x6D7A, 0x6DFB, 0x6E7C, 0x6EFD, 0x6F7E, 0x6FFF,
 0xF060, 0xF0E1, 0xF162, 0xF1E3, 0xF264, 0xF2E5, 0xF366, 0xF3E7,
 0xF468, 0xF4E9, 0xF56A, 0xF5EB, 0xF66C, 0xF6ED, 0xF76E, 0xF7EF,
 0xF870, 0xF8F1, 0xF972, 0xF9F3, 0xFA74, 0xFAF5, 0xFB76, 0xFBF7,
 0xFC78, 0xFCF9, 0xFD7A, 0xFDFB, 0xFE7C, 0xFEFD, 0xFF7E, 0xFFFF 
};

char buffer[512];
void errorf(char *format, ...) {
   *buffer = 0;
   va_list aptr;

   va_start(aptr, format);

   vsprintf(buffer, format, aptr);
   if (convpng.log) { fprintf(convpng.log, "[error line %d] %s", convpng.curline, buffer); }
   fprintf(stderr, "[error line %d] %s", convpng.curline, buffer);
   
   va_end(aptr);
   exit(1);
}

void lof(char *format, ...) {
   *buffer = 0;
   va_list aptr;

   va_start(aptr, format);
   
   vsprintf(buffer, format, aptr);
   if (convpng.log) { fprintf(convpng.log, "%s", buffer); }
   fprintf(stdout, "%s", buffer);
   
   va_end(aptr);
}

inline uint16_t rgb1555(const uint8_t r8, const uint8_t g8, const uint8_t b8) {
    uint8_t r5 = round((int)r8 * 31 / 255.0);
    uint8_t g6 = round((int)g8 * 63 / 255.0);
    uint8_t b5 = round((int)b8 * 31 / 255.0);
    return ((g6 & 1) << 15) | (r5 << 10) | ((g6 >> 1) << 5) | b5;
}

int main(int argc, char **argv) {
    char opt;
    char *ini = NULL;
    char *ext = NULL;
    time_t c1 = time(NULL);
    convpng.iconc = NULL;
    convpng.bad_conversion = false;
    
    while ( (opt = getopt(argc, argv, "c:i:") ) != -1) {
        switch (opt) {
            case 'c':	/* generate an icon header file useable with the C toolchain */
                convpng.iconc = strdup(optarg);
                return create_icon();
            case 'i':	/* change the ini file input */
                ini = (char*)malloc(sizeof(char)*(strlen(optarg)+5));
                strcpy(ini, optarg);
                ext = strrchr(ini,'.');
                if (ext == NULL) {
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
        group[s].sprite = NULL;
        group[s].name = NULL;
        group[s].tindex = -1;
        group[s].numsprites = 0;
        group[s].mode = 0;
    }
    
    if (!convpng.ini) { lof("[error] could not find file '%s'.\nPlease make sure you have created the configuration file\n",ini_file);  exit(1); }
    if (!convpng.log) { lof("could not open file '%s'.\nPlease check file permissions\n",log_file); exit(1); }
    
    lof("Opened %s\n", ini_file);
    
    while ((convpng.line = get_line(convpng.ini))) {
        int num = parse_input();
        free_args(&convpng.line, &convpng.argv, num);
    }
    fclose(convpng.ini);

    lof("\n");
    
    /* Convert all the groups */
    for(g = 0; g < convpng.numgroups; g++) {
        liq_image *image = NULL;
        liq_result *res = NULL;
        liq_attr *attr = NULL;
        
        lof("--- Group %s (%s) ---\n",group[g].name,group[g].mode == MODE_ASM ? "ASM" : "C");
        
        convpng.all_gfx_c = fopen(group[g].outc,"w");
        convpng.all_gfx_h = fopen(group[g].outh,"w");
        if (!convpng.all_gfx_c || !convpng.all_gfx_h) { errorf("could not open %s for output.", group[g].name); }
        
        lof("Building Palette...\n");
        
        for(s = 0; s < group[g].numsprites; s++) {
            unsigned error;
            
            /* open the file */
            error = lodepng_decode32_file(&group[g].sprite[s]->rgba, &group[g].sprite[s]->width, &group[g].sprite[s]->height, group[g].sprite[s]->in);
            if(error) { errorf("%s: %s", lodepng_error_text(error),group[g].sprite[s]->in); }
            
            group[g].sprite[s]->size = group[g].sprite[s]->width*group[g].sprite[s]->height;
            add_rgba(group[g].sprite[s]->rgba, group[g].sprite[s]->size<<2);
            
            /* free the opened image */
            free(group[g].sprite[s]->rgba);
        }
        
        attr = liq_attr_create();
	if(!attr) { errorf("could not create image attributes."); }
	
        /* build the palette */
        image = liq_image_create_rgba(attr, convpng.all_rgba, convpng.all_rgba_size/4, 1, 0);
        if (group[g].tindex >= 0) {
            liq_image_add_fixed_color(image,group[g].tcolor);
        }
        if (!image) { errorf("could not create palette."); }
        res = liq_quantize_image(attr, image);
        if (!res) {errorf("could not quantize palette."); }
        liq_palette *pal = (liq_palette *)liq_get_palette(res);
        
	/* find the transparent color */
        if (group[g].tindex >= 0) {
            for (j = 0; j < 256 ; j++) {
                if (group[g].tcolor_converted == rgb1555(pal->entries[j].r,pal->entries[j].g,pal->entries[j].b)) {
                    group[g].tindex = j;
                }
            }
	    /* move transparent color to index 0 */
	    liq_color tmpp = pal->entries[group[g].tindex];
            pal->entries[group[g].tindex] = pal->entries[0];
            pal->entries[0] = tmpp;
        }
        
        unsigned count = (unsigned)(group[g].tindex == -1) ? 256 : group[g].tindex+1;
        /* now, write the all_gfx output */
        if (group[g].mode == MODE_C) {
            fprintf(convpng.all_gfx_c, "// Converted using ConvPNG\n");
            fprintf(convpng.all_gfx_c, "#include <stdint.h>\n");
            fprintf(convpng.all_gfx_c, "#include \"%s\"\n\n",group[g].outh);
            fprintf(convpng.all_gfx_c, "uint16_t %s_pal[%u] = {\n",group[g].name,count);
	    
            for(j = 0; j < count; j++) {
                fprintf(convpng.all_gfx_c, " 0x%04X%c  // %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(pal->entries[j].r,pal->entries[j].g,pal->entries[j].b),j==255 ? ' ' : ',', j,pal->entries[j].r,pal->entries[j].g,pal->entries[j].b,pal->entries[j].a);
            }
            fprintf(convpng.all_gfx_c, "};");

            fprintf(convpng.all_gfx_h, "// Converted using ConvPNG\n");
            fprintf(convpng.all_gfx_h, "// This file contains all the graphics sources for easier inclusion in a project\n");
            fprintf(convpng.all_gfx_h, "#ifndef %s_H\n#define %s_H\n",group[g].name,group[g].name);
            fprintf(convpng.all_gfx_h, "#include <stdint.h>\n\n");
	    // group[g].tindex
        } else {
            fprintf(convpng.all_gfx_c, "; Converted using ConvPNG\n");
            fprintf(convpng.all_gfx_c, "; This file contains all the graphics for easier inclusion in a project\n\n");
            fprintf(convpng.all_gfx_c, "_%s_pal_size equ %u\n",group[g].name,count<<1);
            fprintf(convpng.all_gfx_c, "_%s_pal:\n",group[g].name);
	    
            for(j = 1; j < count; j++) {
                fprintf(convpng.all_gfx_c, " dw 0%04Xh ; %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(pal->entries[j].r,pal->entries[j].g,pal->entries[j].b),j,pal->entries[j].r,pal->entries[j].g,pal->entries[j].b,pal->entries[j].a);
            }

            fprintf(convpng.all_gfx_h, "; Converted using ConvPNG\n");
            fprintf(convpng.all_gfx_h, "; This file contains all the graphics for easier inclusion in a project\n");
            fprintf(convpng.all_gfx_h, "#ifndef %s_H\n#define %s_H\n\n",group[g].name,group[g].name);
            fprintf(convpng.all_gfx_h, "; ZDS sillyness\n#define db .db\n#define dw .dw\n#define dl .dl\n\n");
            if (group[g].tindex >= 0){
                fprintf(convpng.all_gfx_h, "%s_transpcolor_index equ %d\n\n",group[g].name,group[g].tindex);
            }
            fprintf(convpng.all_gfx_h, "#include \"%s\"\n",group[g].outc);
        }
        
        double diff = liq_get_quantization_error(res);
        if (diff > 10) {
            convpng.bad_conversion = true;
        }
	
        lof("Palette quality : %.2f%%\n",100-diff);
        if (group[g].tindex >= 0) {
            lof("TranspColorIndex : 0x00\n");
            lof("TranspColor : 0x%04X\n%d:\n",group[g].tcolor_converted,group[g].numsprites);
        }

        /* okay, now we have the palette used for all the images. Let's fix them to the attributes */
        for(s = 0; s < group[g].numsprites; s++) {
            unsigned error;
            liq_image *sp_image = NULL;
            liq_result *sp_res = NULL;
            liq_attr *sp_attr = liq_attr_create();
            
            /* open the file */
            error = lodepng_decode32_file(&group[g].sprite[s]->rgba, &group[g].sprite[s]->width, &group[g].sprite[s]->height, group[g].sprite[s]->in);
            if(error) { errorf("%s: %s", lodepng_error_text(error),group[g].sprite[s]->in); }
            
            group[g].sprite[s]->size = group[g].sprite[s]->width*group[g].sprite[s]->height;
            group[g].sprite[s]->data = (uint8_t*)malloc(group[g].sprite[s]->size+1);
            
            sp_image = liq_image_create_rgba(sp_attr, group[g].sprite[s]->rgba, group[g].sprite[s]->width, group[g].sprite[s]->height, 0);
            if (!sp_image) { errorf("could not create image."); }
            for(j = 0; j < 256; j++) {
                liq_image_add_fixed_color(sp_image,pal->entries[j]);
            }
            sp_res = liq_quantize_image(attr, sp_image);
            if (!sp_res) {errorf("could not quantize image."); }
            
            liq_write_remapped_image(sp_res, sp_image, group[g].sprite[s]->data, group[g].sprite[s]->size);

            diff = liq_get_quantization_error(sp_res);
            if (diff > 12) {
                convpng.bad_conversion = true;
            }
        
            lof(" %s quality : %.2f%%",group[g].sprite[s]->name,100-diff);
	    
            /* open the outputs */
            outc = fopen(group[g].sprite[s]->outc,"w");

            if (!outc) { errorf("opening file for output."); }
            
            if (group[g].mode == MODE_C) {
                fprintf(outc,"// Converted using ConvPNG\n");
                fprintf(outc,"#include <stdint.h>\n#include \"%s\"\n\n",group[g].outh);
            } else {
                fprintf(outc,"; Converted using ConvPNG\n\n");
            }
            
            if (group[g].mode == MODE_ASM) {
                fprintf(outc,"_%s_width equ %u\n",group[g].sprite[s]->name,group[g].sprite[s]->width);
                fprintf(outc,"_%s_height equ %u\n",group[g].sprite[s]->name,group[g].sprite[s]->height);
            }
            
            if (group[g].compression != CMP_NONE) {
                unsigned compressed_size;
                uint8_t *data = (uint8_t*)malloc(group[g].sprite[s]->size*2);
                switch(group[g].compression) {
                    case CMP_LZ:
                        compressed_size = (unsigned)LZ_Compress(group[g].sprite[s]->data,data,group[g].sprite[s]->size);
                        break;
                    default:
                        compressed_size = (unsigned)RLE_Compress(group[g].sprite[s]->data,data,group[g].sprite[s]->size);
                        break;
                }
                if (group[g].mode == MODE_C) {
                    fprintf(outc,"uint8_t %s[%u] = {\n ",group[g].sprite[s]->name,compressed_size);
                } else {
                    fprintf(outc,"_%s_size equ %u\n",group[g].sprite[s]->name,compressed_size);
                    fprintf(outc,"_%s:\n db ",group[g].sprite[s]->name);
                }
                for(j = 0; j < compressed_size; j++) {
                    if (group[g].mode == MODE_C) {
                        fprintf(outc,"0x%02X%c",data[j],j+1 == compressed_size ? ' ' : ',');
                    } else {
                        fprintf(outc,"%02X%c",data[j],j+1 == compressed_size ? ' ' : ',');
                    }
                }
                if (group[g].mode == MODE_C) {
                    fprintf(outc,"\n};\n");
                }
                lof(" (compression: %u -> %d bytes) (%s)\n",group[g].sprite[s]->size,compressed_size,group[g].sprite[s]->outc);
                group[g].sprite[s]->size = compressed_size;
                free(data);
            } else {
                lof(" (%s)\n",group[g].sprite[s]->outc);
                if (group[g].mode == MODE_C) {
                    fprintf(outc,"uint8_t %s_data[%u] = {\n %u,\t/* width */\n %u,\t/* height */\n ",group[g].sprite[s]->name,group[g].sprite[s]->size+2,group[g].sprite[s]->width,group[g].sprite[s]->height);
                } else {
                    fprintf(outc,"_%s: ; %u bytes\n db ",group[g].sprite[s]->name,group[g].sprite[s]->size);
                }
                for(j = 0; j < group[g].sprite[s]->height ; j++) {
                    int offset = j*group[g].sprite[s]->width;
                    for(k = 0; k < group[g].sprite[s]->width ; k++) {
                        if (group[g].mode == MODE_C) {
                            char sp = (j+1 == group[g].sprite[s]->height && k+1 == group[g].sprite[s]->width) ? ' ' : ',';
                            fprintf(outc,"0x%02X%c",group[g].sprite[s]->data[k+offset],sp);
                        } else {
                            char sp = k+1 == group[g].sprite[s]->width ? ' ' : ',';
                            fprintf(outc,"0%02Xh%c",group[g].sprite[s]->data[k+offset],sp);
                        }
                    }
                    if (group[g].mode == MODE_C) {
                        fprintf(outc,"\n%s",j+1 != group[g].sprite[s]->height ? " " : "};\n");
                    } else {
                        fprintf(outc,"\n%s",j+1 != group[g].sprite[s]->height ? " db " : "\n");
                    }
                }
            }
            
            if (group[g].mode == MODE_C) {
                fprintf(convpng.all_gfx_h, "extern uint8_t %s_data[%u];\n",group[g].sprite[s]->name,group[g].sprite[s]->size+2);
		fprintf(convpng.all_gfx_h, "#define %s ((gfx_sprite_t*)%s_data)\n",group[g].sprite[s]->name,group[g].sprite[s]->name);
            } else {
                fprintf(convpng.all_gfx_h, "#include \"%s\" ; %u bytes\n",group[g].sprite[s]->outc,group[g].sprite[s]->size);
            }

            /* close the outputs */
            fclose(outc);
            
            /* free the opened image */
            free(group[g].sprite[s]->rgba);
            free(group[g].sprite[s]->data);
            liq_attr_destroy(sp_attr);
            liq_result_destroy(sp_res);
            liq_image_destroy(sp_image);
        }

        fprintf(convpng.all_gfx_h, "extern uint16_t %s_pal[%u];\n",group[g].name,count);
        fprintf(convpng.all_gfx_h, "\n#endif\n");
        
        free(convpng.all_rgba);
        free(group[g].name);
        free(group[g].outc);
        free(group[g].outh);
        liq_attr_destroy(attr);
        liq_image_destroy(image);
        fclose(convpng.all_gfx_h);
        fclose(convpng.all_gfx_c);
        convpng.all_rgba_size = 0;
        convpng.all_rgba = NULL;
        lof("\n");
    }
    lof("Converted in %u s\n\n",(unsigned)(time(NULL)-c1));
    if(convpng.bad_conversion) {
        lof("[warning] the quality may be too low. (90+%% reccomended).\nPlease try grouping similar images or reducing image colors.\n\n");
    }
    free(ini);
    lof("Finished!\n");
    
    return 0;
}

/**
 * frees the allocated memory for later use
 */
void free_args(char **args, char ***argv, unsigned argc) {
    if (argv) {
	unsigned i;
        for(i=0; i<argc; i++) {
            free((*argv)[i]);
            (*argv)[i] = NULL;
        }
        free(*argv);
        *argv = NULL;
    }
    if (*args) {
        free(*args);
        *args = NULL;
    }
}

/**
 * recieve string
 */
char *get_line(FILE *stream) {   
    char *line = (char*)malloc(sizeof(char));
    char *tmp;
    if (feof(stream)) {
        free(line);
        return NULL;
    }

    if (line) {
	int i = 0, c = EOF;
        while (c) {
            c = fgetc(stream);
            
            if (c == '\n' || c == EOF) {
                c = '\0';
            }
            
            if (c != ' ') {
                line[i++] = (char)c;
                tmp = realloc(line, sizeof(char)*(i+1));
                if (tmp == NULL) {
                    errorf("fatal error");
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
    
    if (token == NULL) {
        return -1;
    }
    
    /* while there is a space present */
    while(token != NULL) {
        size_t strl = strlen(token);
        
        /* allocate the memory then copy it in */
        *args = realloc(*args, sizeof(char*) * (argc + 1));
        (*args)[argc] = (char*)malloc(sizeof(char ) * (strl + 1));
        memcpy((*args)[argc], token, strl+1);
        
        /* increment the argument number */
        argc++;

        /* get the next token */
        token = strtok(NULL, delim);
    }
    *args = realloc(*args, sizeof(char*) * (argc + 1));
    (*args)[argc] = NULL;
    return argc;
}

void add_sprite(char *line) {
    int g = convpng.numgroups - 1;
    int s = group[g].numsprites;
    char *ext;
    size_t strl = strlen(line) + 1;
    
    if (!(group[g].sprite = realloc(group[g].sprite, sizeof(sprite_t*) * (s + 1))))       goto err;
    if (!(group[g].sprite[s] = (sprite_t*)malloc(sizeof(sprite_t))))                      goto err;
    if (!(group[g].sprite[s]->pal = (uint8_t*)malloc(256)))                               goto err;
    
    if (!(group[g].sprite[s]->in = (char*)malloc(sizeof(char)*(strl+5))))                 goto err;
    strcpy(group[g].sprite[s]->in, line);
    ext = strrchr(group[g].sprite[s]->in,'.');
    if (ext == NULL) {
        strcat(group[g].sprite[s]->in,".png");
        ext = strrchr(group[g].sprite[s]->in,'.');
    }
 
    if (!(group[g].sprite[s]->outc = (char*)malloc(sizeof(char)*(strlen(group[g].sprite[s]->in)+5))))  goto err;
    strcpy(group[g].sprite[s]->outc,group[g].sprite[s]->in);
    strcpy(group[g].sprite[s]->outc+(ext-group[g].sprite[s]->in),group[g].mode == MODE_C ? ".c" : ".asm");
    
    if (!(group[g].sprite[s]->name = (char*)malloc(sizeof(char)*(strlen(group[g].sprite[s]->in)+1))))  goto err;
    strcpy(group[g].sprite[s]->name, group[g].sprite[s]->in);
    group[g].sprite[s]->name[(int)(strrchr(group[g].sprite[s]->name,'.')-group[g].sprite[s]->name)] = '\0';
    
    group[g].numsprites++;
    
    return;

err:
    errorf("internal memory allocation error. please contact developer.\n");
}

void print_sprites(void) {
    int i;
    
    for (i = 0; i < group[0].numsprites; i++) {
        lof("  %d  %s\n", i+1, group[0].sprite[i]->in);
    }
}

int parse_input(void) {
    int num = 0;
    if (convpng.line[0] != '\0') {   
        if (convpng.line[0] == '#') {
            num = make_args(convpng.line, &convpng.argv, ":");
            
            /* Set the transparent color */
            if (!strcmp(convpng.argv[0], "#TranspColor")) {
                int g = convpng.numgroups - 1;
                char **colors;
                if (num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = make_args(convpng.argv[1], &colors, ",");
                if (num < 4) { errorf("not enough colors."); }
                
                group[g].tcolor.r = (uint8_t)atoi(colors[0]);
                group[g].tcolor.g = (uint8_t)atoi(colors[1]);
                group[g].tcolor.b = (uint8_t)atoi(colors[2]);
                group[g].tcolor.a = (uint8_t)atoi(colors[3]);
                group[g].tcolor_converted = rgb1555(group[g].tcolor.r,group[g].tcolor.g,group[g].tcolor.b);
                group[g].tindex = 0;
                
                /* Free the allocated memory */
                free_args(&convpng.argv[1], &colors, num);
            }
            
            if (!strcmp(convpng.argv[0], "#Sprites")) {
            }
            
            if (!strcmp(convpng.argv[0], "#Compression")) {
                if (!strcmp(convpng.argv[1], "rle")) {
                    group[convpng.numgroups-1].compression = CMP_RLE;
                }
                if (!strcmp(convpng.argv[1], "lz77")) {
                    group[convpng.numgroups-1].compression = CMP_LZ;
                }
            }
            
            if (!strcmp(convpng.argv[0], "#GroupC")) {
                int g = convpng.numgroups;
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
            
            if (!strcmp(convpng.argv[0], "#GroupASM")) {
                int g = convpng.numgroups;
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
            add_sprite(convpng.line);
        }
    }
    return num;
}

void add_rgba(uint8_t *pal, size_t size) {
    convpng.all_rgba = realloc(convpng.all_rgba, sizeof(uint8_t)*(convpng.all_rgba_size+size+1));
    memcpy(convpng.all_rgba+convpng.all_rgba_size,pal,size);
    convpng.all_rgba_size += size;
}

/**
 * create an icon for the C toolchain
 * Perform no image quantization; force the user to use the correct palette so they can learn something
 */
int create_icon(void) {
    uint8_t *rgb;
    unsigned width,height,size,error,i,k,o,x,y;
    uint8_t *image;
    error = lodepng_decode24_file(&rgb, &width, &height, "iconc.png");
    if(error) { lof("[error] could not open 'iconc.png' for conversion\n"); exit(1); }

    if (width != 16 || height != 16) { errorf("icon image dimensions are not 16x16."); }
    
    o = 0;
    size = width*height;
    image = (uint8_t*)malloc(sizeof(uint8_t)*size);
    
    for(i = 0; i < size; i++) {
            uint16_t pxlcolor = rgb1555(rgb[o],rgb[o+1],rgb[o+2]);
            for(k = 0; k < 256 && hilo[k] != pxlcolor; ++k);
            image[i] = k;
            o += 3;
    }

    FILE *out = fopen("iconc.asm","w");
    fprintf(out," define .icon,space=ram\n segment .icon\n xdef __icon_begin\n xdef __icon_end\n xdef __program_description\n xdef __program_description_end\n");
    
    fprintf(out,"\n db 1\n db %u,%u\n__icon_begin:\n db ",width,height);
    for(y = 0; y < height; y++) {
        fputs("\n db ",out);
        for(x = 0; x < width; x++) {
            fprintf(out,"0%02Xh%s",image[x+(y*width)],x+1==width ? "" : ",");
        }
    }
                        
    fprintf(out,"\n__icon_end:\n__program_description:\n");
    fprintf(out," db \"%s\",0\n__program_description_end:\n",convpng.iconc);

    lof("Converted 'iconc.png'\n");
    free(convpng.iconc);
    free(image);
    fclose(out);
    return 0;
}