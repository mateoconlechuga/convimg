#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#include "lodepng.h"

unsigned error;

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
    int fileindex;
} input;

typedef struct {
	unsigned int size;
	unsigned short *data;
} palette_t;

struct image_t {
	char *name;
	
	unsigned char* pal_image;
	unsigned char* rgba_data;
    unsigned char* pal_image_data;
	unsigned short* raw_image;
	unsigned short* color;
	unsigned int width, height;
	unsigned int size;
	palette_t palette;
} image;

struct palette_t {
	unsigned int size;
	unsigned short *data;
} palette;

static inline unsigned int read24bitInt(const unsigned char* buffer) {
  return (unsigned int)((buffer[0]<<16) | (buffer[1]<<8) | buffer[2]);
}

static inline unsigned short irgb1555(const unsigned c) {
    return (unsigned short)(((c>>9)&0x7C00) | ((c>>6)&0x03E0) | ((c>>16)&0x1F));
}

static inline unsigned short rgb565(const unsigned char r, const unsigned char g, const unsigned char b){
	return (unsigned short)((b>>3) | ((g>>2)<<5) | ((r>>3)<<11));
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
    
    unsigned short currpixelcolor;
    
    /* get the palette used first */
    offset=0;
    image.palette.size = 1;
    /* using external image as the palette */
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

int convertImage() {
	/* variable declarations */
	unsigned i,x,y;
	
	image.name = malloc(strlen(input.name));
	char *tmp;
	
	tmp = strrchr( input.name, '/' );
	if(tmp == NULL) { tmp = strrchr( input.name, '\\' ); }
	if(tmp == NULL) { tmp = input.name; }
	
	strcpy(image.name,tmp);
	image.name[(int)(strrchr( image.name, '.' )-image.name)] = '\0';
	printf("%s\n",image.name);
    
    error = lodepng_decode24_file(&image.rgba_data, &image.width, &image.height, input.name);
    
	if(error) {
		printf("error %u: %s\n", error, lodepng_error_text(error));
	}

    /* get the size of the image */
	image.size = image.width*image.height;
	
    /* make sure the image isn't too large */
	if(image.width > 0xFF || image.height > 0xFF) {
		fprintf(stderr, "error: image too large.\n");
		//return -1;
	}
	
    /* if not 16bpp, we need a palette */
	if(input.bppmode != 16) {
		/* allocate block for array */
		image.pal_image = (unsigned char*)malloc(image.size+1);
		image.palette.data = (unsigned short*)malloc(0x200);
        
		/* convert the image */
		if((error = get1555())) {
			return error;
		}
	
		if(image.palette.data == NULL) {
			printf("error: image uses too many colors\n");
			return -1;
		}
	} else {
		image.raw_image = (unsigned short*)malloc(image.size<<1);
		get565();
	}
	
	/* open the output file */
    output.file = fopen( output.name, (output.overwrite) ? "w" : "a" );
	if ( !output.file ) {
			fprintf(stderr, "error: unable to open output file.\n");
			return -1;
	} 
    
    /* if not 16bpp, we need a palette */
	if(input.bppmode != 16) { 
        output.usepal = true;
    }
    
    /* write the palette to the file */
	if(output.usepal == true && input.makeicon == false && output.write_palette == true) {
		fprintf(output.file,"\n_%s_pal_start\n",(output.custompalette = true) ? "lcd" : image.name);
		for(i=0;i<image.palette.size;i++) {
			fprintf(output.file," dw 0%04Xh\t; 0x%02X\n",image.palette.data[i],i);
		}
		fprintf(output.file,"_%s_pal_end\n",(output.custompalette = true) ? "lcd" : image.name);
	}
	
    /* return if we only need to generate the palette */
    if(output.onlypal) {
        return 0;
    }
    
	if(input.makeicon == true) {
		fprintf(output.file," segment .icon\n jp __icon_end");
		if(image.width > 16 || image.height > 16) {
			fprintf(stderr, "error: invalid icon dimensions.\n");
			//return -1;
		}
		fprintf(output.file,"\n db 1\n db %d,%d", image.width, image.height);
	} else {
		fprintf(output.file,"\n_%s_start",image.name);
		fprintf(output.file,"\n db %d,%d", image.width, image.height);
	}
	
	switch(input.bppmode) {
		case 8:
				for(y=0;y<image.height;y++) {
					fprintf(output.file,"\n db ");
					for(x=0;x<image.width;x++) {
						fprintf(output.file,"0%02Xh%s",image.pal_image[x+(y*image.width)],((x==image.width-1) ? "" : ","));
					}
				}
				break;
		case 16:
				for(y=0;y<image.height;y++) {
					fprintf(output.file,"\n dw ");
					for(x=0;x<image.width;x++) {
						fprintf(output.file,"0%04Xh%s",image.raw_image[x+(y*image.width)],((x==image.width-1) ? "" : ","));
					}
				}
				break;
		default:
				break;
	}
	
	if(input.makeicon == true) {
		fprintf(output.file,"\n__icon_end\n");
	} else {
		fprintf(output.file,"\n_%s_end\n",image.name);
	}
	
	fclose(output.file);
    free(image.name);
    free(image.pal_image);
    free(image.raw_image);
    free(image.palette.data);
	return 0;
}

int main(int argc, char* argv[]) {
	/* variable declarations */
	char *ext;

    int opt,i,numfilestoconvert;
	
    bool writting_pal_to_file = false;
    
	input.bppmode = 16;	/* default is 16bpp */
	input.makeicon = false;
	input.hilo = false;
	
    input.fileindex = numfilestoconvert = 0;
    output.name = input.palette_name;
    output.write_palette = true;
    output.custompalette = output.overwrite = output.onlypal = false;
    
	if(argc > 1)
    {
        while ( (opt = getopt(argc, argv, "8i:p:o:g:zcj") ) != -1)
        {
            switch (opt)
            {
				case '8':	/* convert to 8bpp, generate palette */
						input.bppmode = 8;
						break;
				case 'i':	/* generate an icon header file useable with the C toolchain */
                        optind--;
                        for( ;optind < argc && ( (*(*(argv+optind)) != '-') && (strlen(argv[optind]) != 2)); optind++){
                              numfilestoconvert++;
                              if(input.fileindex == 0) {
                                 input.fileindex = optind;
                              }
                        }
						break;
                case 'p':
                        input.palette_name = optarg;
                        output.custompalette = true;
                        break;
                case 'g':
                        input.name = malloc( strlen( optarg )+5 );
                        strcpy(input.name, optarg);
                        output.onlypal = true;
                        break;
                case 'o':
                        output.name = optarg;
                        break;
                case 'z':
						output.overwrite = true;
						break;
                case 'c':	/* generate an icon header file useable with the C toolchain */
						input.makeicon = true;
						input.hilo = true;
						input.bppmode = 8;
                        break;
                case 'j':
                        writting_pal_to_file = true;
                        break;
                case 'h':	/* generate an icon header file useable with the C toolchain */
						input.hilo = true;
						input.bppmode = 8;
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
        printf("\ti <image file>: [image file] is the file to convert\n");
        printf("\tp <palette>: Use as the palette for <image file>\n");
        printf("\tg <palette>: Just output the palette for <palette image>\n");
        printf("\to <output file>: Write output to <output file>\n");
        printf("\t8: Output in 8bpp format (Default is 16bpp)\n");
        printf("\tz: Overwrite output file (Default is append)\n");
        printf("\tc: Create icon for C toolchain (output is written to icon.asm)\n");
        printf("\tj: Use with -p; outputs <palette> as well to file\n");
        return -1;
    }

    output.write_palette = writting_pal_to_file;
    
    for(i=0;i<numfilestoconvert;++i) {
        input.name = malloc( strlen( argv[input.fileindex] )+5 );
        strcpy(input.name, argv[input.fileindex]);
                            
        /* change the extension if it exists; otherwise create a new one */
        ext = strrchr( input.name, '.' );
        if( ext == NULL ) {
            strcat( input.name, ".png" );
            ext = strrchr( input.name, '.' );
        }
       
        /* create the output name */
        if( output.name == NULL ) {
            output.name = malloc( strlen( input.name )+5 );
            strcpy( output.name, input.name );
            strcpy( output.name+(ext-input.name), ".asm");
        }

        /* print out some debug things */
        printf("Input File: %s\nOutput File: %s\n", input.name, output.name);
        
        if( (error = convertImage()) ) {
            return error;
        }
        
        input.fileindex++;
        free(input.name);
        output.write_palette = output.overwrite = false;
    }
	
	/* free the output/input name buffers */
    free(output.name);
    free(input.name);
        
	return error;
}