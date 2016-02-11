#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

#include "lodepng.h"

int error;

uint16_t highlow_pal[256] = {
    0x0000,    /* 0x00 */
    0x0081,    /* 0x01 */
    0x0102,    /* 0x02 */
    0x0183,    /* 0x03 */
    0x0204,    /* 0x04 */
    0x0285,    /* 0x05 */
    0x0306,    /* 0x06 */
    0x0387,    /* 0x07 */
    0x0408,    /* 0x08 */
    0x0489,    /* 0x09 */
    0x050A,    /* 0x0A */
    0x058B,    /* 0x0B */
    0x060C,    /* 0x0C */
    0x068D,    /* 0x0D */
    0x070E,    /* 0x0E */
    0x078F,    /* 0x0F */
    0x0810,    /* 0x10 */
    0x0891,    /* 0x11 */
    0x0912,    /* 0x12 */
    0x0993,    /* 0x13 */
    0x0A14,    /* 0x14 */
    0x0A95,    /* 0x15 */
    0x0B16,    /* 0x16 */
    0x0B97,    /* 0x17 */
    0x0C18,    /* 0x18 */
    0x0C99,    /* 0x19 */
    0x0D1A,    /* 0x1A */
    0x0D9B,    /* 0x1B */
    0x0E1C,    /* 0x1C */
    0x0E9D,    /* 0x1D */
    0x0F1E,    /* 0x1E */
    0x0F9F,    /* 0x1F */
    0x1000,    /* 0x20 */
    0x1081,    /* 0x21 */
    0x1102,    /* 0x22 */
    0x1183,    /* 0x23 */
    0x1204,    /* 0x24 */
    0x1285,    /* 0x25 */
    0x1306,    /* 0x26 */
    0x1387,    /* 0x27 */
    0x1408,    /* 0x28 */
    0x1489,    /* 0x29 */
    0x150A,    /* 0x2A */
    0x158B,    /* 0x2B */
    0x160C,    /* 0x2C */
    0x168D,    /* 0x2D */
    0x170E,    /* 0x2E */
    0x178F,    /* 0x2F */
    0x1810,    /* 0x30 */
    0x1891,    /* 0x31 */
    0x1912,    /* 0x32 */
    0x1993,    /* 0x33 */
    0x1A14,    /* 0x34 */
    0x1A95,    /* 0x35 */
    0x1B16,    /* 0x36 */
    0x1B97,    /* 0x37 */
    0x1C18,    /* 0x38 */
    0x1C99,    /* 0x39 */
    0x1D1A,    /* 0x3A */
    0x1D9B,    /* 0x3B */
    0x1E1C,    /* 0x3C */
    0x1E9D,    /* 0x3D */
    0x1F1E,    /* 0x3E */
    0x1F9F,    /* 0x3F */
    0x2020,    /* 0x40 */
    0x20A1,    /* 0x41 */
    0x2122,    /* 0x42 */
    0x21A3,    /* 0x43 */
    0x2224,    /* 0x44 */
    0x22A5,    /* 0x45 */
    0x2326,    /* 0x46 */
    0x23A7,    /* 0x47 */
    0x2428,    /* 0x48 */
    0x24A9,    /* 0x49 */
    0x252A,    /* 0x4A */
    0x25AB,    /* 0x4B */
    0x262C,    /* 0x4C */
    0x26AD,    /* 0x4D */
    0x272E,    /* 0x4E */
    0x27AF,    /* 0x4F */
    0x2830,    /* 0x50 */
    0x28B1,    /* 0x51 */
    0x2932,    /* 0x52 */
    0x29B3,    /* 0x53 */
    0x2A34,    /* 0x54 */
    0x2AB5,    /* 0x55 */
    0x2B36,    /* 0x56 */
    0x2BB7,    /* 0x57 */
    0x2C38,    /* 0x58 */
    0x2CB9,    /* 0x59 */
    0x2D3A,    /* 0x5A */
    0x2DBB,    /* 0x5B */
    0x2E3C,    /* 0x5C */
    0x2EBD,    /* 0x5D */
    0x2F3E,    /* 0x5E */
    0x2FBF,    /* 0x5F */
    0x3020,    /* 0x60 */
    0x30A1,    /* 0x61 */
    0x3122,    /* 0x62 */
    0x31A3,    /* 0x63 */
    0x3224,    /* 0x64 */
    0x32A5,    /* 0x65 */
    0x3326,    /* 0x66 */
    0x33A7,    /* 0x67 */
    0x3428,    /* 0x68 */
    0x34A9,    /* 0x69 */
    0x352A,    /* 0x6A */
    0x35AB,    /* 0x6B */
    0x362C,    /* 0x6C */
    0x36AD,    /* 0x6D */
    0x372E,    /* 0x6E */
    0x37AF,    /* 0x6F */
    0x3830,    /* 0x70 */
    0x38B1,    /* 0x71 */
    0x3932,    /* 0x72 */
    0x39B3,    /* 0x73 */
    0x3A34,    /* 0x74 */
    0x3AB5,    /* 0x75 */
    0x3B36,    /* 0x76 */
    0x3BB7,    /* 0x77 */
    0x3C38,    /* 0x78 */
    0x3CB9,    /* 0x79 */
    0x3D3A,    /* 0x7A */
    0x3DBB,    /* 0x7B */
    0x3E3C,    /* 0x7C */
    0x3EBD,    /* 0x7D */
    0x3F3E,    /* 0x7E */
    0x3FBF,    /* 0x7F */
    0x4040,    /* 0x80 */
    0x40C1,    /* 0x81 */
    0x4142,    /* 0x82 */
    0x41C3,    /* 0x83 */
    0x4244,    /* 0x84 */
    0x42C5,    /* 0x85 */
    0x4346,    /* 0x86 */
    0x43C7,    /* 0x87 */
    0x4448,    /* 0x88 */
    0x44C9,    /* 0x89 */
    0x454A,    /* 0x8A */
    0x45CB,    /* 0x8B */
    0x464C,    /* 0x8C */
    0x46CD,    /* 0x8D */
    0x474E,    /* 0x8E */
    0x47CF,    /* 0x8F */
    0x4850,    /* 0x90 */
    0x48D1,    /* 0x91 */
    0x4952,    /* 0x92 */
    0x49D3,    /* 0x93 */
    0x4A54,    /* 0x94 */
    0x4AD5,    /* 0x95 */
    0x4B56,    /* 0x96 */
    0x4BD7,    /* 0x97 */
    0x4C58,    /* 0x98 */
    0x4CD9,    /* 0x99 */
    0x4D5A,    /* 0x9A */
    0x4DDB,    /* 0x9B */
    0x4E5C,    /* 0x9C */
    0x4EDD,    /* 0x9D */
    0x4F5E,    /* 0x9E */
    0x4FDF,    /* 0x9F */
    0x5040,    /* 0xA0 */
    0x50C1,    /* 0xA1 */
    0x5142,    /* 0xA2 */
    0x51C3,    /* 0xA3 */
    0x5244,    /* 0xA4 */
    0x52C5,    /* 0xA5 */
    0x5346,    /* 0xA6 */
    0x53C7,    /* 0xA7 */
    0x5448,    /* 0xA8 */
    0x54C9,    /* 0xA9 */
    0x554A,    /* 0xAA */
    0x55CB,    /* 0xAB */
    0x564C,    /* 0xAC */
    0x56CD,    /* 0xAD */
    0x574E,    /* 0xAE */
    0x57CF,    /* 0xAF */
    0x5850,    /* 0xB0 */
    0x58D1,    /* 0xB1 */
    0x5952,    /* 0xB2 */
    0x59D3,    /* 0xB3 */
    0x5A54,    /* 0xB4 */
    0x5AD5,    /* 0xB5 */
    0x5B56,    /* 0xB6 */
    0x5BD7,    /* 0xB7 */
    0x5C58,    /* 0xB8 */
    0x5CD9,    /* 0xB9 */
    0x5D5A,    /* 0xBA */
    0x5DDB,    /* 0xBB */
    0x5E5C,    /* 0xBC */
    0x5EDD,    /* 0xBD */
    0x5F5E,    /* 0xBE */
    0x5FDF,    /* 0xBF */
    0x6060,    /* 0xC0 */
    0x60E1,    /* 0xC1 */
    0x6162,    /* 0xC2 */
    0x61E3,    /* 0xC3 */
    0x6264,    /* 0xC4 */
    0x62E5,    /* 0xC5 */
    0x6366,    /* 0xC6 */
    0x63E7,    /* 0xC7 */
    0x6468,    /* 0xC8 */
    0x64E9,    /* 0xC9 */
    0x656A,    /* 0xCA */
    0x65EB,    /* 0xCB */
    0x666C,    /* 0xCC */
    0x66ED,    /* 0xCD */
    0x676E,    /* 0xCE */
    0x67EF,    /* 0xCF */
    0x6870,    /* 0xD0 */
    0x68F1,    /* 0xD1 */
    0x6972,    /* 0xD2 */
    0x69F3,    /* 0xD3 */
    0x6A74,    /* 0xD4 */
    0x6AF5,    /* 0xD5 */
    0x6B76,    /* 0xD6 */
    0x6BF7,    /* 0xD7 */
    0x6C78,    /* 0xD8 */
    0x6CF9,    /* 0xD9 */
    0x6D7A,    /* 0xDA */
    0x6DFB,    /* 0xDB */
    0x6E7C,    /* 0xDC */
    0x6EFD,    /* 0xDD */
    0x6F7E,    /* 0xDE */
    0x6FFF,    /* 0xDF */
    0x7060,    /* 0xE0 */
    0x70E1,    /* 0xE1 */
    0x7162,    /* 0xE2 */
    0x71E3,    /* 0xE3 */
    0x7264,    /* 0xE4 */
    0x72E5,    /* 0xE5 */
    0x7366,    /* 0xE6 */
    0x73E7,    /* 0xE7 */
    0x7468,    /* 0xE8 */
    0x74E9,    /* 0xE9 */
    0x756A,    /* 0xEA */
    0x75EB,    /* 0xEB */
    0x766C,    /* 0xEC */
    0x76ED,    /* 0xED */
    0x776E,    /* 0xEE */
    0x77EF,    /* 0xEF */
    0x7870,    /* 0xF0 */
    0x78F1,    /* 0xF1 */
    0x7972,    /* 0xF2 */
    0x79F3,    /* 0xF3 */
    0x7A74,    /* 0xF4 */
    0x7AF5,    /* 0xF5 */
    0x7B76,    /* 0xF6 */
    0x7BF7,    /* 0xF7 */
    0x7C78,    /* 0xF8 */
    0x7CF9,    /* 0xF9 */
    0x7D7A,    /* 0xFA */
    0x7DFB,    /* 0xFB */
    0x7E7C,    /* 0xFC */
    0x7EFD,    /* 0xFD */
    0x7F7E,    /* 0xFE */
    0x7FFF    /* 0xFF */
};

/* global definitions */
struct output_t {
    bool makeappvar;
    bool usepal;
    bool maketilemap;
    bool overwrite;
    bool onlypal;
    bool write_palette;
    bool custompalette;
    FILE *file;
    char *name;
} output;

struct input_t {
    char *name;
    char *palette_name;
    unsigned char bppmode;
    bool makeicon;
    bool hilo;
    bool make_c_header;
    int fileindex;
    char *icon_description;
} input;

typedef struct {
    unsigned int size;
    uint16_t *data;
} palette_t;

struct image_t {
    char *name;
    
    uint8_t* pal_image;
    uint8_t* rgba_data;
    uint8_t* pal_image_data;
    uint16_t* raw_image;
    uint16_t* color;
    unsigned int width, height;
    unsigned int size;
    palette_t palette;
} image;

static inline unsigned int read24bitInt(const unsigned char* buffer) {
    return (unsigned int)((buffer[0]<<16) | (buffer[1]<<8) | buffer[2]);
}

static inline uint16_t irgb1555(const unsigned c) {
    uint8_t r = (c>>16)&0xFF;
    uint8_t g = (c>>8)&0xFF;
    uint8_t b = (c)&0xFF;
    return (uint16_t)((r >> 3) << 10) | (((g >> 3) << 5) | (b >> 3));
}

static inline uint16_t rgb565(const uint8_t r, const uint8_t g, const uint8_t b){
    return (uint16_t)((b>>3) | ((g>>2)<<5) | ((r>>3)<<11));
}

void get565(void) {
    unsigned int i,offset;
    
    offset=0;
    for(i=0;i<image.size;i++) {
        image.raw_image[i] = rgb565( image.rgba_data[offset], image.rgba_data[offset+1], image.rgba_data[offset+2] );
        offset += 3;
    }
}

/* returns a generated palette given the raw image data (ARGB->1555) */
int get1555(void) {
    unsigned int offset,i,k;
    
    unsigned int tmp_width, tmp_height;
    
    uint16_t currpixelcolor;
    
    /* get the palette used first */
    offset=0;
    image.palette.size = 1;
    /* using external image as the palette */
    if(input.makeicon) {
	image.palette.size = 256;
        image.palette.data = highlow_pal;
	offset = 0;
        for(i=0;i<image.size;i++) {
            currpixelcolor = irgb1555( read24bitInt(image.rgba_data+offset) );
            for(k=0;k<image.palette.size && image.palette.data[k] != currpixelcolor;++k);
            image.pal_image[i] = k;
            offset += 3;
        }
	return 0;
    }
    if( output.custompalette == true ) {
        if((error = lodepng_decode24_file(&image.pal_image_data, &tmp_width, &tmp_height, input.palette_name))) {
            return error;
        }
        
        image.palette.data[0] = irgb1555( read24bitInt(image.pal_image_data) );
        
        for(i=0;i<tmp_height*tmp_width;i++) {
            currpixelcolor = irgb1555( read24bitInt(image.pal_image_data+offset) );
            
            for(k=0;k<image.palette.size && image.palette.data[k] != currpixelcolor;++k);
            if(k == image.palette.size) {
                image.palette.data[image.palette.size++] = currpixelcolor;
            }
            
            offset += 3;
        }
        
        /* get the palettized image */
        offset = 0;
        for(i=0;i<image.size;i++) {
            currpixelcolor = irgb1555( read24bitInt(image.rgba_data+offset) );
            for(k=0;k<image.palette.size && image.palette.data[k] != currpixelcolor;++k);
            image.pal_image[i] = k;
            offset += 3;
        }
        /* using current image as the palette */
    } else {
        image.palette.data[0] = irgb1555( read24bitInt(image.rgba_data) );
        for(i=0;i<image.size;i++) {
            currpixelcolor = irgb1555( read24bitInt(image.rgba_data+offset) );
            
            for(k=0;k<image.palette.size && image.palette.data[k] != currpixelcolor;++k);
            if(k == image.palette.size) {
                image.palette.data[image.palette.size++] = currpixelcolor;
            }
            image.pal_image[i] = k;
            offset += 3;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    /* variable declarations */
    char *ext;
    unsigned i,x,y,numfilestoconvert;
    int opt;
    char *tmp;
    
    bool writting_pal_to_file = false;
    
    /* default is 16bpp */
    input.bppmode = 16;
    input.makeicon = false;
    input.hilo = false;
    input.make_c_header = false;
    
    input.fileindex = numfilestoconvert = 0;
    output.name = input.palette_name;
    output.write_palette = true;
    output.custompalette = output.overwrite = output.onlypal = false;
    
    if(argc > 1)
    {
        while ( (opt = getopt(argc, argv, "8i:p:o:g:c:zjh") ) != -1)
        {
            switch (opt)
            {
                case '8':	/* convert to 8bpp, generate palette */
                    input.bppmode = 8;
                    break;
                case 'i':   /* specify input files */
                    optind--;
                    for( ;optind < argc && ( (*(*(argv+optind)) != '-') && (strlen(argv[optind]) != 2)); optind++){
                        numfilestoconvert++;
                        if(input.fileindex == 0) {
                            input.fileindex = optind;
                        }
                    }
                    break;
                case 'p':   /* use the following palette to generate the image */
                    input.palette_name = optarg;
                    output.custompalette = true;
                    break;
                case 'g':   /* only output the palette of the image */
                    input.name = malloc( strlen( optarg )+5 );
                    strcpy(input.name, optarg);
                    output.onlypal = true;
                    break;
                case 'o':   /* change output name */
                    output.name = optarg;
                    break;
                case 'z':   /* overwrite output file */
                    output.overwrite = true;
                    break;
                case 'c':	/* generate an icon header file useable with the C toolchain */
                    input.makeicon = true;
                    input.hilo = true;
                    input.bppmode = 8;
		    input.icon_description = optarg;
		    output.overwrite = true;
                    break;
                case 'j':   /* write the palette to the file as well */
                    writting_pal_to_file = true;
                    break;
                case 'h':	/* generate a header file for C, rather than an ASM file */
                    input.make_c_header = true;
                    input.makeicon = false;
                    break;
                default:
                    printf("Unrecognized option: '%c'\n", opt);
                    goto show_help;
                    break;
            }
        }
    } else {
    show_help:
        printf("ConvImage Utility v1.00 - by M. Waltz\n");
        printf("\nGenerates formats useable for the Primecell PL111 controller\n");
        printf("\nUsage:\n\tconvpng [-options]");
        printf("\nOptions:\n");
        printf("\ti <image file>: [image file] is the file to convert (Can be multiple)\n");
        printf("\tp <palette>: Use as the palette for <image file>\n");
        printf("\tg <palette>: Just output the palette for <palette image>\n");
        printf("\to <output file>: Write output to <output file>\n");
	printf("\tc <description>: Create icon for C toolchain (output is written to iconc.asm, along with the description)\n");
        printf("\t8: Output in 8bpp format (Default is 16bpp)\n");
        printf("\tz: Overwrite output file (Default is append)\n");
        printf("\tj: Use with -p; outputs <palette> as well to file\n");
        printf("\th: Create a C header file rather than an ASM file\n");
        return -1;
    }
    
    output.write_palette = writting_pal_to_file;
    
    for(i=0;i<numfilestoconvert;++i) {
        input.name = malloc(strlen(argv[input.fileindex])+5);
        strcpy(input.name,argv[input.fileindex]);
        
        /* change the extension if it exists; otherwise create a new one */
        ext = strrchr(input.name,'.');
        if( ext == NULL ) {
            strcat(input.name,".png");
            ext = strrchr(input.name,'.');
        }
        
        /* create the output name */
        if( output.name == NULL ) {
            output.name = malloc(strlen( input.name )+5);
            strcpy(output.name,input.name);
            strcpy(output.name+(ext-input.name),(input.make_c_header) ? ".h" : ".asm");
        }
        
        image.name = malloc(strlen(input.name));
        
        tmp = strrchr( input.name, '/' );
        if(tmp == NULL) { tmp = strrchr( input.name, '\\' ); }
        if(tmp == NULL) { tmp = input.name; }
        
        strcpy(image.name,tmp);
        image.name[(int)(strrchr( image.name, '.' )-image.name)] = '\0';
        
	/* open the file */
        error = lodepng_decode24_file(&image.rgba_data, &image.width, &image.height, input.name);
        if(error) {
            fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
	    return error;
        }
        
        /* get the size of the image */
        image.size = image.width*image.height;
        
        /* if not 16bpp, we need a palette */
        if(input.bppmode != 16) {
            /* allocate block for array */
            image.pal_image = (uint8_t*)malloc(image.size+1);
            image.palette.data = (uint16_t*)malloc(0x200);
            
            /* convert the image */
            if((error = get1555())) {
                return error;
            }
            
            if(image.palette.data == NULL) {
                fprintf(stderr,"%s","error: image uses too many colors\n");
                return -1;
            }
        } else {
            image.raw_image = (uint16_t*)malloc(image.size<<1);
            get565();
        }
        
        /* open the output file */
        output.file = fopen( output.name, (output.overwrite) ? "w" : "a" );
        if ( !output.file ) {
            fprintf(stderr,"%s","error: unable to open output file.\n");
            return -1;
        }
        
        /* if not 16bpp, we need a palette */
        if(input.bppmode != 16) {
            output.usepal = true;
        }
        
        /* Write some comments */
        if(input.make_c_header) {
            fprintf(output.file,"#ifndef %s_h\n#define %s_h\n\n/* Converted using ConvPNG */\n",image.name,image.name);
        } else {
            fprintf(output.file,"%s","; Converted using ConvPNG ;\n");
        }
        
        /* write the palette */
        if(((output.usepal == true && input.makeicon == false) || (output.write_palette == true)) && (input.bppmode==8)) {
            /* write the palette to the header file */
            if(input.make_c_header) {
                fprintf(output.file,"\nunsigned short int %s_pal[%d] = {\n",(output.custompalette == true) ? "lcd" : image.name,image.palette.size);
                for(i=0;i<image.palette.size;i++) {
                    fprintf(output.file,"    0x%04X%s    /* 0x%02X */\n",image.palette.data[i],(i==image.palette.size-1) ? "" : ",",i);
                }
                fprintf(output.file,"};");
                /* write the palette to the asm file */
            } else {
                fprintf(output.file,"\n_%s_pal_start\n",(output.custompalette == true) ? "lcd" : image.name);
                for(i=0;i<image.palette.size;i++) {
                    fprintf(output.file," dw 0%04Xh\t; 0x%02X\n",image.palette.data[i],i);
                }
                fprintf(output.file,"_%s_pal_end\n",(output.custompalette == true) ? "lcd" : image.name);
            }
        }
        
        /* return if we only need to generate the palette */
        if(output.onlypal) {
            return 0;
        }
        
        if(input.makeicon == true) {
            fprintf(output.file,"%s"," define .icon,space=ram\n segment .icon\n xdef __icon_begin\n xdef __icon_end\n xdef __program_description");
            if(image.width > 16 || image.height > 16) {
                fprintf(stderr,"%s","error: invalid icon dimensions.\n");
                return -1;
            }
            fprintf(output.file,"\n db 1\n db %d,%d\n__icon_begin:", image.width, image.height);
        } else {
            /* write the header information */
            if(input.make_c_header) {
                fprintf(output.file,"\n%s %s[%d] = {",(input.bppmode==8) ? "unsigned char" : "unsigned short int",image.name,image.width*image.height);
            } else {
                fprintf(output.file,"\n_%s_start",image.name);
                fprintf(output.file,"\n db %d,%d", image.width, image.height);
            }
        }
        
        switch(input.bppmode) {
            case 8:
                for(y=0;y<image.height;y++) {
                    if(input.make_c_header) {
                        fprintf(output.file,"\n    ");
                        for(x=0;x<image.width;x++) {
                            fprintf(output.file,"0x%02X%s",image.pal_image[x+(y*image.width)], (y==image.height-1 && x==image.width-1) ? "" : ",");
                        }
                    } else {
                        fprintf(output.file,"\n db ");
                        for(x=0;x<image.width;x++) {
                            fprintf(output.file,"0%02Xh%s",image.pal_image[x+(y*image.width)],(x==image.width-1) ? "" : ",");
                        }
                    }
                }
                break;
            case 16:
                for(y=0;y<image.height;y++) {
                    if(input.make_c_header) {
                        fprintf(output.file,"%s","\n    ");
                        for(x=0;x<image.width;x++) {
                            fprintf(output.file,"0x%04X%s",image.raw_image[x+(y*image.width)],(y==image.height-1 && x==image.width-1) ? "" : ",");
                        }
                    } else {
                        fprintf(output.file,"\n dw ");
                        for(x=0;x<image.width;x++) {
                            fprintf(output.file,"0%04Xh%s",image.raw_image[x+(y*image.width)],(x==image.width-1) ? "" : ",");
                        }
                    }
                }
                break;
            default:
                break;
        }
        
        if(input.makeicon == true) {
            fprintf(output.file,"\n__icon_end:\n__program_description:\n");
	    fprintf(output.file," db \"%s\",0\n",input.icon_description);
        } else {
            if(input.make_c_header) {
                fprintf(output.file,"\n};\n\n#endif\n");
            } else {
                fprintf(output.file,"\n_%s_end\n",image.name);
            }
        }
        
        /* print out some things */
        printf("%s > %s\n", input.name, output.name);
        
        input.fileindex++;
        free(input.name);
	fclose(output.file);
        free(image.name);
        free(image.pal_image);
        free(image.raw_image);
        free(image.palette.data);
        output.write_palette = output.overwrite = false;
    }
    
    /* free the output/input name buffers */
    free(output.name);
    free(input.name);
    
    return error;
}
