#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "libs/libimagequant.h"
#include "libs/lodepng.h"

#include "main.h"
#include "misc.h"
#include "appvar.h"
#include "logging.h"

// get a string
char *get_line(FILE *stream) {   
    char *line = safe_malloc(1);
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
                tmp = safe_realloc(line, i+1);
                if(!tmp) {
                    errorf("memory error.");
                } else {
                    line = tmp;
                }
            }
        }
        convpng.curline++;
    }
    
    return line;
}

int separate_args(char *srcstr, char ***output, const char sep) {
	int i, len = strlen(srcstr);
	int numparts = 0;
	char **currentpart;

	for (i = 0; i < len; i++)
	{
		if (srcstr[i] == sep)
		{
			srcstr[i] = '\0';	
			numparts++;
		}
	}

	numparts++;
	*output = malloc(numparts*sizeof(char*));

	if (*output == NULL) {
		errorf("memory error.");
	}
	
	currentpart = *output;
	*currentpart = srcstr;

	for (i = 0; i < len; i++) {
		if (srcstr[i] == '\0') {
			currentpart++;
			*currentpart = &(srcstr[i+1]);
		}
	}
	
	return numparts; 
}

// adds an image to the indexed array
static void add_image(char *line) {
    group_t *g = &group[convpng.numgroups - 1]; image_t *s;
    unsigned k = g->numimages;
    char *ext;
    
    // allocate memory for the new image
    g->image = safe_realloc(g->image, sizeof(image_t*) * (k + 1));
    s = g->image[k] = safe_malloc(sizeof(image_t));
    
    // add the .png extension if needed
    s->in = safe_malloc(strlen(line)+5);
    strcpy(s->in, line);
    if(!(ext = strrchr(s->in,'.'))) {
        strcat(s->in,".png");
        ext = strrchr(s->in,'.');
    }
 
    // do the whole thing where you create output names
    s->outc = safe_malloc(strlen(s->in)+5);
    strcpy(s->outc, s->in);
    strcpy(s->outc+(ext-(s->in)), g->mode == MODE_C ? ".c" : ".asm");
    
    // create the name of the file
    s->name = safe_malloc(strlen(s->in)+1);
    strcpy(s->name, s->in);
    s->name[(int)(strrchr(s->name,'.')-s->name)] = '\0';
    
    // increment the number of images we have
    g->numimages++;
}

// parse the ini file
int parse_input(char *line) {
    int num = 0;
    if(*line != '\0') {   
        if(*line == '#') {
            char **argv;
            group_t *g = &group[convpng.numgroups - 1];
            num = separate_args(line, &argv, ':');

            // set the transparent color
            if(!strcmp(*argv, "#TranspColor")) {
                char **colors;
                
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = separate_args(argv[1], &colors, ',');
                if(num < 4) { errorf("not enough transparency colors."); }

                g->tcolor.r = (uint8_t)strtol(colors[0], NULL, 10);
                g->tcolor.g = (uint8_t)strtol(colors[1], NULL, 10);
                g->tcolor.b = (uint8_t)strtol(colors[2], NULL, 10);
                g->tcolor.a = (uint8_t)strtol(colors[3], NULL, 10);
                g->tcolor_converted = rgb1555(g->tcolor.r, g->tcolor.g, g->tcolor.b);
                g->use_tcolor = true;
                
                // free the allocated memory
                free(colors);
            }
            
            // add a transparent index color
            if(!strcmp(*argv, "#TranspIndex")) {
                char **index;
                
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = separate_args(argv[1], &index, ',');
                if(num < 1) { errorf("invalid transparency index"); }
                
                g->tindex = (unsigned)strtol(*index, NULL, 10);
                g->use_tindex = true;
                
                // free the allocated memory
                free(index);
            }
            
            if(!strcmp(*argv, "#AppVar")) {
                appvar_t *a = &appvar[convpng.numappvars];
                a->g = g = &group[convpng.numgroups];
                g->mode = MODE_APPVAR;
                memset(a->name, 0, 9);
                strncpy(a->name, argv[1], 8);
                convpng.numappvars++;
                convpng.numgroups++;
            }
        
            if(!strcmp(*argv, "#Compression")) {
                if(!strcmp(argv[1], "zx7")) {
                    g->compression = COMPRESS_ZX7;
                }
            }
            
            if(!strcmp(*argv, "#CreateGlobalPalette")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->is_global_palette = true;
                g->mode = MODE_C;
                convpng.numgroups++;
            }
        
            if(!strcmp(*argv, "#Palette")) {
                g->palette_name = str_dup(argv[1]);
            }
            
            if(!strcmp(*argv, "#OutputPaletteImage")) {
                g->output_palette_image = true;
            }
            
            if(!strcmp(*argv, "#NoPaletteArray")) {
                g->output_palette_array = false;
            }
            
            if(!strcmp(*argv, "#Tilemap")) {
                char **tilemap_options;
        
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = separate_args(argv[1], &tilemap_options, ',');
                if(num < 3) { errorf("not enough options specified (tile_width,tile_hieght)."); }
                
                g->tile_width = (unsigned)strtol(tilemap_options[0], NULL, 10);
                g->tile_height = (unsigned)strtol(tilemap_options[1], NULL, 10);
                g->create_tilemap_ptrs = !strcmp(tilemap_options[2], "true");
                g->tile_size = g->tile_width * g->tile_height + 2;
                g->convert_to_tilemap = true;
                
                // free the allocated memory
                free(tilemap_options);
            }
        
            if(!strcmp(*argv, "#BitsPerPixel")) {
                uint8_t bpp = (uint8_t)strtol(argv[1], NULL, 10);
                switch(bpp) {
                    case 1: case 2: case 4: case 8: case 16:
                        break;
                    default:
                        errorf("unsupported bpp mode selected.");
                        break;
                }
                g->bpp = bpp;
            }
            
            // A C conversion type
            if(!strcmp(*argv, "#GroupC")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcat(argv[1], ".h");
                g->outc = str_dupcat(argv[1], ".c");
                g->mode = MODE_C;
                convpng.numgroups++;
            }

            // An ASM group
            if(!strcmp(*argv, "#GroupASM")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcat(argv[1], ".inc");
                g->outc = str_dupcat(argv[1], ".asm");
                g->mode = MODE_ASM;
                convpng.numgroups++;
            }
            
            // output in ICE format
            if(!strcmp(*argv, "#GroupICE")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcat(argv[1], ".txt");
                g->outc = str_dupcat(argv[1], ".txt");
                g->mode = MODE_ICE;
                convpng.numgroups++;
            }
            
            // free the args
            free(argv);

        } else {
            add_image(line);
        }
    }
    return num;
}
