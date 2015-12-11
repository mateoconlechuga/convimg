#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#define LODEPNG_NO_COMPILE_ENCODER
#include "lodepng.h"

unsigned error;

/* global definitions */
struct output_t {
	bool makeappvar;
	bool usepal;
	bool maketilemap;
	FILE *file;
	char *name;
} output;

struct input_t {
	char *name;
	unsigned char bppmode;
	bool makeicon;
	bool hilo;
} input;

typedef struct {
	unsigned int size;
	unsigned short *data;
} palette_t;

struct image_t {
	char *name;
	
	unsigned char* pal_image;
	unsigned char* rgba_data;
	unsigned short* raw_image;
	unsigned short* color;
	unsigned int width, height;
	unsigned int size;
	palette_t palette;
} image;

struct palette_t {
	unsigned size;
	unsigned short *data;
} palette;

static void generateHILOpalette() {
  unsigned short b;
  unsigned char d;
  unsigned char a;
  unsigned char e;
 
  for(b=0;b<0x100;b++)
  {
      d = b;
      a = b;
      a &= 0xC0;
      a = a>>1;
      a |= (d&1)<<7;
      d = d>>1;
      e = a;
      a = 0x1F;
      a &= b;
      a |= e;
      image.palette.data[b] = a | (d<<8);
  }
}

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
	
	image.color = (unsigned short*)malloc(image.size<<1);

	offset=0;
	for(i=0;i<image.size;i++) {
		image.color[i] = irgb1555( read24bitInt(image.rgba_data+offset) );
		offset += 3;
	}

	if(input.hilo == false) {
		image.palette.size = 1;
		image.palette.data[0] = image.color[0];
		for(i=0;i<image.size;i++) {
			for(k=0;k<image.palette.size && image.palette.data[k] != image.color[i];++k);
			if(k == image.palette.size) { image.palette.data[image.palette.size++] = image.color[i]; }
			image.pal_image[i] = k;

			if(image.palette.size>0x100) {
				return -1;
			}
		}
	} else {
		generateHILOpalette();
		for(i=0;i<image.size;i++) {
			for(k=0;k<image.palette.size && image.palette.data[k] != image.color[i];++k);
			image.pal_image[i] = k;
		}
	}
	return 0;
}

int convertImage() {
	/* variable declarations */
	unsigned i,x,y;
	
	image.name = malloc(strlen(output.name));
	char *tmp;
	
	tmp = strrchr( output.name, '/' );
	if(tmp == NULL) { tmp = strrchr( output.name, '\\' ); }
	if(tmp == NULL) { tmp = output.name; }
	
	strcpy(image.name,tmp);
	image.name[(int)(strrchr( image.name, '.' )-image.name)] = '\0';
	printf("%s\n",image.name);
	
	error = lodepng_decode24_file(&image.rgba_data, &image.width, &image.height, input.name);
	if(error) {
		printf("error %u: %s\n", error, lodepng_error_text(error));
	}

	image.size = image.width*image.height;
	
	if(image.size > 76800) {
		fprintf(stderr, "error: image dimensions too large.\n");
		//return -1;
	}
	
	if(input.bppmode != 16) {
		/* allocate block for array */
		image.pal_image = (unsigned char*)malloc(image.size+1);
		image.palette.data = (unsigned short*)malloc(0x100<<1);
		
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
	output.file = fopen( output.name, "w" );
	if ( !output.file ) {
			fprintf(stderr, "error: unable to open output file.\n");
			return -1;
	}
	if(input.bppmode != 16) { output.usepal = true; }
    
	if(output.usepal == true && input.makeicon == false) {
		fprintf(output.file,"_%s_pal_start\n",image.name);
		for(i=0;i<image.palette.size;i++) {
			fprintf(output.file," dw 0%04Xh\t; 0x%02X\n",image.palette.data[i],i);
		}
		fprintf(output.file,"_%s_pal_end\n",image.name);
	}
	
	if(input.makeicon == true) {
		fprintf(output.file," segment .icon\n jp __icon_end");
		if(image.width > 16 || image.height > 16) {
			fprintf(stderr, "error: invalid icon dimensions.\n");
			//return -1;
		}
		fprintf(output.file,"\n db 1\n db %d,%d", image.width, image.height);
	} else {
		fprintf(output.file,"_%s_start",image.name);
		fprintf(output.file,"\n dl %d,%d", image.width, image.height);
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
	
	if(input.makeicon == true) {
		output.file = fopen( "icon.h", "w" );
		fprintf(output.file,"/* icon header file */\n\n");
		fprintf(output.file,"#pragma asm \"#include \\\"%s.asm\\\"\"\n\n",image.name);
		fprintf(output.file,"/* end icon header file */");
		fclose(output.file);
	}

	return 0;
}

int main(int argc, char* argv[]) {
	/* variable declarations */
	char *ext;
    
    int opt;
	
	input.bppmode = 16;	/* default is 16bpp */
	input.makeicon = false;
	input.hilo = false;
	
	if(argc > 1)
    {
        while ( (opt = getopt(argc, argv, "8ic:") ) != -1)
        {
            switch (opt)
            {
				case '8':	/* convert to 8bpp, generate palette */
						input.bppmode = 8;
						break;
				case 'i':	/* generate an icon header file useable with the C toolchain */
						input.makeicon = true;
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
        printf("ConvPNG Utility v1.00 - by M. Waltz\n");
        printf("\nGenerates formats useable for the Primecell PL111 controller\n");
        printf("\nUsage:\n\tconvpng [-options] <filename>");
        printf("\nOptions:\n");
        printf("\t8: Output in 8bpp format (Default is 16bpp)\n");
        return -1;
    }
	
	/* get the filenames for both out and in files */
	input.name = malloc( strlen( argv[argc-1] )+5 );
	strcpy(input.name, argv[argc-1]);
    
    /* change the extension if it exists; otherwise create a new one */
    ext = strrchr( input.name, '.' );
    if( ext == NULL ) {
        strcat( input.name, ".png" );
        ext = strrchr( input.name, '.' );
    }
   
	/* create the output name */
    output.name = malloc( strlen( input.name )+5 );
    strcpy( output.name, input.name );
    strcpy( output.name+(ext-input.name), ".asm");

	/* print out some debug things */
    printf("Input File: %s\nOutput File: %s\n", input.name, output.name);
	
	error = convertImage();
	
	/* free the output.name buffer */

	return error;
}