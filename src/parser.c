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

// free allocated memory
void free_args(char **args, char ***argv, unsigned argc) {
    if(argv) {
        unsigned i;
        for (i=0; i<argc; i++) {
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

// get a string
char *get_line(FILE *stream) {   
    char *line = safe_malloc(sizeof(char));
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
                tmp = safe_realloc(line, sizeof(char)*(i+1));
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

// makes a character string pointer array
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
        *args = safe_realloc(*args, sizeof(char*) * (argc + 1));
        (*args)[argc] = safe_malloc(sizeof(char ) * (strl + 1));
        memcpy((*args)[argc], token, strl+1);
        
        // increment the argument number
        argc++;

        // get the next token
        token = strtok(NULL, delim);
    }
    *args = safe_realloc(*args, sizeof(char*) * (argc + 1));
    (*args)[argc] = NULL;
    return argc;
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
            group_t *g = &group[convpng.numgroups - 1];
            num = make_args(line, &convpng.argv, ":");
            
            // set the transparent color
            if(!strcmp(*convpng.argv, "#TranspColor")) {
                char **colors;
                
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = make_args(convpng.argv[1], &colors, ",");
                if(num < 4) { errorf("not enough transparency colors."); }

                g->tcolor.r = (uint8_t)strtol(colors[0], NULL, 10);
                g->tcolor.g = (uint8_t)strtol(colors[1], NULL, 10);
                g->tcolor.b = (uint8_t)strtol(colors[2], NULL, 10);
                g->tcolor.a = (uint8_t)strtol(colors[3], NULL, 10);
                g->tcolor_converted = rgb1555(g->tcolor.r, g->tcolor.g, g->tcolor.b);
                g->use_tcolor = true;
                
                // free the allocated memory
                free_args(NULL, &colors, num);
            }
            
            // add a transparent index color
            if(!strcmp(*convpng.argv, "#TranspIndex")) {
                char **index;
                
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = make_args(convpng.argv[1], &index, ",");
                if(num < 1) { errorf("invalid transparency index"); }
                
                g->tindex = (unsigned)strtol(*index, NULL, 10);
                g->use_tindex = true;
                
                // free the allocated memory
                free_args(NULL, &index, num);
            }
            
            if(!strcmp(*convpng.argv, "#AppVar")) {
                appvar_t *a = &appvar[convpng.numappvars];
                a->g = g = &group[convpng.numgroups];
                g->mode = MODE_APPVAR;
                memset(a->name, 0, 9);
                strncpy(a->name, convpng.argv[1], 8);
                convpng.numappvars++;
                convpng.numgroups++;
            }
        
            if(!strcmp(*convpng.argv, "#Compression")) {
                if(!strcmp(convpng.argv[1], "zx7")) {
                    g->compression = COMPRESS_ZX7;
                }
            }
            
            if(!strcmp(*convpng.argv, "#CreateGlobalPalette")) {
                g = &group[convpng.numgroups];
                g->is_global_palette = true;
                g->mode = MODE_C;
                g->name = safe_malloc(strlen(convpng.argv[1])+1);
                strcpy(g->name, convpng.argv[1]);
                convpng.numgroups++;
            }
        
            if(!strcmp(*convpng.argv, "#Palette")) {
                g->palette_name = safe_malloc(strlen(convpng.argv[1])+1);
                strcpy(g->palette_name, convpng.argv[1]);
            }
            
            if(!strcmp(*convpng.argv, "#OutputPaletteImage")) {
                g->output_palette_image = true;
            }
            
            if(!strcmp(*convpng.argv, "#NoPaletteArray")) {
                g->output_palette_array = false;
            }
            
            if(!strcmp(*convpng.argv, "#Tilemap")) {
                char **tilemap_options;
        
                if(num <= 1) { errorf("parsing line %d", convpng.curline); }
                num = make_args(convpng.argv[1], &tilemap_options, ",");
                if(num < 3) { errorf("not enough options specified (tile_width,tile_hieght)."); }
                
                g->tile_width = (unsigned)strtol(tilemap_options[0], NULL, 10);
                g->tile_height = (unsigned)strtol(tilemap_options[1], NULL, 10);
                g->create_tilemap_ptrs = !strcmp(tilemap_options[2], "true");
                g->tile_size = g->tile_width * g->tile_height + 2;
                g->convert_to_tilemap = true;
                
                /* Free the allocated memory */
                free_args(NULL, &tilemap_options, num);
            }
        
            if(!strcmp(*convpng.argv, "#BitsPerPixel")) {
                uint8_t bpp = (uint8_t)strtol(convpng.argv[1], NULL, 10);
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
            if(!strcmp(*convpng.argv, "#GroupC")) {
                g = &group[convpng.numgroups];
                g->name = safe_malloc(strlen(convpng.argv[1])+1);
                strcpy(g->name,convpng.argv[1]);
                g->outh = safe_malloc(strlen(convpng.argv[1])+3);
                strcpy(g->outh,convpng.argv[1]);
                strcat(g->outh,".h");
                g->outc = safe_malloc(strlen(convpng.argv[1])+3);
                strcpy(g->outc,convpng.argv[1]);
                strcat(g->outc,".c");
                g->mode = MODE_C;
                convpng.numgroups++;
            }

            // An ASM group
            if(!strcmp(*convpng.argv, "#GroupASM")) {
                g = &group[convpng.numgroups];
                g->name = safe_malloc(strlen(convpng.argv[1])+1);
                strcpy(g->name,convpng.argv[1]);
                g->outh = safe_malloc(strlen(convpng.argv[1])+5);
                strcpy(g->outh,convpng.argv[1]);
                strcat(g->outh,".inc");
                g->outc = safe_malloc(strlen(convpng.argv[1])+5);
                strcpy(g->outc,convpng.argv[1]);
                strcat(g->outc,".asm");
                g->mode = MODE_ASM;
                convpng.numgroups++;
            }
            
            // output in ICE format
            if(!strcmp(*convpng.argv, "#GroupICE")) {
                g = &group[convpng.numgroups];
                g->name = safe_malloc(strlen(convpng.argv[1])+1);
                strcpy(g->name,convpng.argv[1]);
                g->outh = safe_malloc(strlen(convpng.argv[1])+5);
                strcpy(g->outh,convpng.argv[1]);
                strcat(g->outh,".txt");
                g->outc = safe_malloc(strlen(convpng.argv[1])+5);
                strcpy(g->outc,convpng.argv[1]);
                strcat(g->outc,".txt");
                g->mode = MODE_ICE;
                convpng.numgroups++;
            }
        
        } else {
            add_image(line);
        }
    }
    return num;
}