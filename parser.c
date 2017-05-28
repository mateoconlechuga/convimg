#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

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

	for (i = 0; i < len; i++) {
		if (srcstr[i] == sep) {
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

// check if there is a wildcard, if so we need to create a list of images to add
static char **find_pngs(DIR* dir, const char *path, unsigned int *len) {
    char **png_array = NULL;
    *len = 0;
    if (dir) {
        unsigned int png_count = 0;
        struct dirent* file;
        
        // find a list of all the png files in the directory
        while ((file = readdir(dir))) {
            const char *name = file->d_name;
            if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
            if (strstr(name, ".png")) {
                png_array = realloc(png_array, (png_count + 1) * sizeof(char *));
                png_array[png_count++] = str_dupcat(path, name);
                *len += 1;
            }
        }
        closedir(dir);
    }
    return png_array;
}

// adds an image to the indexed array
static void add_image(char *line) { 
    group_t *g = &group[convpng.numgroups - 1]; image_t *s;
    unsigned int len = 1;
    unsigned int i;
    bool open_dir = false;
    char **images = NULL;
    char *in;
    DIR* dir;
    
    // check if relative or absolute file path
    if (line[strlen(line)-1] == '/') {
        line[strlen(line)-1] = '\0';
    }
    
    // check for wildcard at end
    if (line[strlen(line)-1] == '*') {
        line[strlen(line)-1] = '\0';
        if (!strlen(line)) {
            *line = '.';
        }
        open_dir = true;
        dir = opendir(line);
        images = find_pngs(dir, line, &len);
    }
    
    for (i = 0; i < len; i++) {
        if (open_dir) { line = images[i]; }
        (in = strrchr(line, '/')) ? in++ : (in = line);
        
        lof ("%s\n", line);
        
        // allocate memory for the new image
        g->image = safe_realloc(g->image, sizeof(image_t*) * (g->numimages + 1));
        s = g->image[g->numimages] = safe_malloc(sizeof(image_t));
        
        // add the .png extension if needed
        if (!strchr(in,'.')) {
            s->in = str_dupcat(line, ".png");
        } else {
            s->in = str_dup(line);
        }
        
        (in = strrchr(s->in, '/')) ? in++ : (in = s->in);
        
        // create the name of the file
        s->name = str_dup(in);
        s->name[(int)(strrchr(s->name,'.')-s->name)] = '\0';
        
        // do the whole thing where you create output names
        s->outc = str_dupcat(s->name, g->mode == MODE_C ? ".c" : ".asm");
        if (convpng.directory) {
            char *free_outc = s->outc;
            s->outc = str_dupcat(convpng.directory, s->outc);
            free(free_outc);
        }
        
        // increment the number of images we have
        g->numimages++;
    }
    
    if (open_dir) {
        for(i = 0; i < len; i++) {
            free(images[i]);
        }
        free(images);
    }
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
            if(!strcmp(*argv, "#TransparentColor") || !strcmp(*argv, "#TranspColor")) {
                char **colors;
                
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = separate_args(argv[1], &colors, ',');
                if(num == 3) {
                    g->tcolor.a = 255;
                    goto add_other_colors;
                } else if(num < 4) {
                    errorf("not enough transparency colors.");
                }
                
                g->tcolor.a = (uint8_t)strtol(colors[3], NULL, 10);
add_other_colors:
                g->tcolor.r = (uint8_t)strtol(colors[0], NULL, 10);
                g->tcolor.g = (uint8_t)strtol(colors[1], NULL, 10);
                g->tcolor.b = (uint8_t)strtol(colors[2], NULL, 10);
                g->tcolor_converted = rgb1555(g->tcolor.r, g->tcolor.g, g->tcolor.b);
                g->use_tcolor = true;
                
                // free the allocated memory
                free(colors);
            }
            
            // add a transparent index color
            if(!strcmp(*argv, "#TransparentIndex") || !strcmp(*argv, "#TranspIndex")) {
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                
                g->tindex = (unsigned)strtol(argv[1], NULL, 10);
                g->use_tindex = true;
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
            
            if(!strcmp(*argv, "#Style")) {
                if(!strcmp(argv[1], "transparent")) {
                    g->style = STYLE_TRANSPARENT;
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
            
            if(!strcmp(*argv, "#OutputPaletteArray")) {
                if(!strcmp(argv[1], "false")) {
                    g->output_palette_array = false;
                } else {
                    g->output_palette_array = true;
                }
            }
            
            if(!strcmp(*argv, "#OutputDirectory")) {
                if (argv[1][strlen(argv[1])-1] != '/') {
                    convpng.directory = str_dupcat(argv[1], "/");
                } else {
                    convpng.directory = str_dup(argv[1]);
                }
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
        
            if(!strcmp(*argv, "#BitsPerPixel") || !strcmp(*argv, "#BPP")) {
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
                char *free_me;
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcat(argv[1], ".h");
                g->outc = str_dupcat(argv[1], ".c");
                if (convpng.directory) {
                    free_me = g->outc;
                    g->outc = str_dupcat(convpng.directory, g->outc);
                    free(free_me);
                    free_me = g->outh;
                    g->outh = str_dupcat(convpng.directory, g->outh);
                    free(free_me);
                }
                g->mode = MODE_C;
                convpng.numgroups++;
            }

            // An ASM group
            if(!strcmp(*argv, "#GroupASM")) {
                char *free_me;
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcat(argv[1], ".inc");
                g->outc = str_dupcat(argv[1], ".asm");
                if (convpng.directory) {
                    free_me = g->outc;
                    g->outc = str_dupcat(convpng.directory, g->outc);
                    free(free_me);
                    free_me = g->outh;
                    g->outh = str_dupcat(convpng.directory, g->outh);
                    free(free_me);
                }
                g->mode = MODE_ASM;
                convpng.numgroups++;
            }
            
            // output in ICE format
            if(!strcmp(*argv, "#GroupICE")) {
                char *free_me;
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outc = str_dupcat(argv[1], ".txt");
                if (convpng.directory) {
                    free_me = g->outc;
                    g->outc = str_dupcat(convpng.directory, g->outc);
                    free(free_me);
                }
                g->outh = NULL;
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
