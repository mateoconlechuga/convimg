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
#include "parser.h"
#include "logging.h"

// completely parses the ini file
void parse_convpng_ini(void) {
    char *line = NULL;
    while ((line = get_line(convpng.ini))) {
        parse_input(line);
        free(line);
    }
    // close the ini file
    fclose(convpng.ini);
    convpng.ini = NULL;
    
    // init the appvars created from parsing
    init_appvars();
    
    // add an extra newline for kicks
    lof("\n");
}

// get a string
char *get_line(FILE *stream) {   
    char *line = NULL;
    char *tmp;

    if(!feof(stream)) {
        line = safe_malloc(1);
        int i = 0, c = EOF;
        while (c) {
            c = fgetc(stream);

            if(c == '\r' || c == '\n' || c == EOF) {
                c = '\0';
            }

            if(c != ' ' && c != '\t') {
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
    group_t *g = &group[convpng.numgroups - 1];
    unsigned int len = 1;
    unsigned int i;
    char *in;
    DIR* dir;
    image_t *s;
    bool open_dir = false;
    char **images = NULL;
    
    // check if relative or absolute file path
    if (line[strlen(line)-1] == '/') {
        line[strlen(line)-1] = '\0';
    }
    
    // check for wildcard at end
    if (line[strlen(line)-1] == '*') {
        line[strlen(line)-1] = '\0';
        if (!strlen(line)) {
            dir = opendir("./");
        } else {
            dir = opendir(line);
        }
        open_dir = true;
        images = find_pngs(dir, line, &len);
    }
    
    for (i = 0; i < len; i++) {
        if (open_dir) { line = images[i]; }
        (in = strrchr(line, '/')) ? in++ : (in = line);
        
        // allocate memory for the new image
        g->image = safe_realloc(g->image, sizeof(image_t*) * (g->numimages + 1));
        s = g->image[g->numimages] = safe_malloc(sizeof(image_t));
        
        // inherit properties from group
        s->compression = g->compression;
        s->style = g->style;
        s->convert_to_tilemap = g->convert_to_tilemap;
        s->create_tilemap_ptrs = g->create_tilemap_ptrs;
        
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

// error if not enough arguments
static void args_error(void) {
   if (convpng.log) { fprintf(convpng.log, "[error] incorrect number of arguments (line %d)\n", convpng.curline); }
   fprintf(stderr, "[error] incorrect number of arguments (line %d)\n", convpng.curline);
   if (convpng.log) { fclose(convpng.log); }
   if (convpng.ini) { fclose(convpng.ini); }
   exit(1);
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
                
                if (num <= 1) { args_error(); }
                num = separate_args(argv[1], &colors, ',');
                if(num == 3) {
                    g->tcolor.a = 255;
                    goto add_other_colors;
                } else if(num < 4) {
                    args_error();
                }
                
                g->tcolor.a = (uint8_t)strtol(colors[3], NULL, 10);
add_other_colors:
                g->tcolor.r = (uint8_t)strtol(colors[0], NULL, 10);
                g->tcolor.g = (uint8_t)strtol(colors[1], NULL, 10);
                g->tcolor.b = (uint8_t)strtol(colors[2], NULL, 10);
                g->use_tcolor = true;
                
                // free the allocated memory
                free(colors);
            } else
            
            // add a transparent index color
            if(!strcmp(*argv, "#TransparentIndex") || !strcmp(*argv, "#TranspIndex")) {
                g->tindex = (unsigned int)strtol(argv[1], NULL, 10);
                g->use_tindex = true;
            } else
            
            // add a fixed color index
            if(!strcmp(*argv, "#FixedIndexColor")) {
                char **colors;
                unsigned int numf = g->num_fixed_colors;
                fixed_t *f = &g->fixed[numf];
                if (numf > 200) {
                    errorf("too many fixed color indexes");
                }
                
                if (num <= 1) { args_error(); }
                num = separate_args(argv[1], &colors, ',');
                if(num == 4) {
                    f->color.a = 255;
                    goto add_other_colors_fixed;
                } else if(num < 5) {
                    args_error();
                }
                
                f->color.a = (uint8_t)strtol(colors[4], NULL, 10);
add_other_colors_fixed:
                f->color.r = (uint8_t)strtol(colors[1], NULL, 10);
                f->color.g = (uint8_t)strtol(colors[2], NULL, 10);
                f->color.b = (uint8_t)strtol(colors[3], NULL, 10);
                f->index = (unsigned int)strtol(colors[0], NULL, 10);
                g->num_fixed_colors++;
                
                // free the allocated memory
                free(colors);
            } else
            
            // palette index that should not be exported to the output image
            if(!strcmp(*argv, "#OmitIndex")) {
                if (g->style == STYLE_RLET) {
                    errorf("cannot use #OmitIndex with rlet style");
                }

                if (num > 2) { args_error(); }

                g->oindex = (uint8_t)strtol(argv[1], NULL, 10);
                g->use_oindex = true;
            } else

            // color that should not be exported to the output image
            if(!strcmp(*argv, "#OmitColor")) {
                char **colors;

                if (g->style == STYLE_RLET) {
                    errorf("cannot use #OmitColor with rlet style");
                }

                if (num <= 1) { args_error(); }
                num = separate_args(argv[1], &colors, ',');
                if(num == 3) {
                    g->ocolor.a = 255;
                    goto add_other_colors_omit;
                } else if(num < 4) {
                    args_error();
                }
                
                g->ocolor.a = (uint8_t)strtol(colors[3], NULL, 10);
add_other_colors_omit:
                g->ocolor.r = (uint8_t)strtol(colors[0], NULL, 10);
                g->ocolor.g = (uint8_t)strtol(colors[1], NULL, 10);
                g->ocolor.b = (uint8_t)strtol(colors[2], NULL, 10);
                g->use_ocolor = true;
                
                // free the allocated memory
                free(colors);
            } else
            
            if(!strcmp(*argv, "#AppvarC")) {
                appvar_t *a = &appvar[convpng.numappvars];
                g = &group[convpng.numgroups];
                memset(a->name, 0, 9);
                strncpy(a->name, argv[1], 8);
                g->mode = MODE_APPVAR;
                g->name = str_dup(a->name);
                g->outh = str_dupcatdir(a->name, ".h");
                g->outc = str_dupcatdir(a->name, ".c");
                a->mode = MODE_C;
                a->g = g;
                convpng.numappvars++;
                convpng.numgroups++;
            } else
            
            if(!strcmp(*argv, "#AppvarICE")) {
                appvar_t *a = &appvar[convpng.numappvars];
                g = &group[convpng.numgroups];
                memset(a->name, 0, 9);
                strncpy(a->name, argv[1], 8);
                g->mode = MODE_APPVAR;
                g->name = str_dup(a->name);
                g->outh = str_dupcatdir(a->name, ".txt");
                g->outc = str_dupcatdir(a->name, ".txt");
                a->mode = MODE_ICE;
                a->g = g;
                convpng.numappvars++;
                convpng.numgroups++;
            } else
                                
            if(!strcmp(*argv, "#AppvarASM")) {
                appvar_t *a = &appvar[convpng.numappvars];
                g = &group[convpng.numgroups];
                memset(a->name, 0, 9);
                strncpy(a->name, argv[1], 8);
                g->mode = MODE_APPVAR;
                g->name = str_dup(a->name);
                g->outh = str_dupcatdir(a->name, ".inc");
                g->outc = str_dupcatdir(a->name, ".asm");
                a->mode = MODE_ASM;
                a->g = g;
                convpng.numappvars++;
                convpng.numgroups++;
            } else
            
            if(!strcmp(*argv, "#OutputInitCode")) {
                appvar_t *a = &appvar[convpng.numappvars - 1];
                if(!strcmp(argv[1], "false")) {
                    a->write_init = false;
                } else {
                    a->write_init = true;
                }
            } else
            
            if(!strcmp(*argv, "#IncludePalettes")) {
                int i;
                char **palettes;
                appvar_t *a = &appvar[convpng.numappvars - 1];
                num = separate_args(argv[1], &palettes, ',');
                if (!num) { args_error(); }
                a->palette = safe_realloc(a->palette, num * sizeof(char*));
                a->palette_data = safe_realloc(a->palette_data, num * sizeof(liq_palette));
                for (i = 0; i < num; i++) {
                    a->palette[i] = str_dup(palettes[i]);
                    a->palette_data[i] = NULL;
                }
                a->numpalettes = num;
                free(palettes);
            } else
            
            if(!strcmp(*argv, "#Compression")) {
                if(!strcmp(argv[1], "zx7")) {
                    g->compression = COMPRESS_ZX7;
                }
            } else
            
            if(!strcmp(*argv, "#Style")) {
                if(!strcmp(argv[1], "rlet")) {
                    if (g->use_ocolor) {
                        errorf("cannot use #OmitColor with rlet style");
                    }
                    g->style = STYLE_RLET;
                }
            } else

            if(!strcmp(*argv, "#CreateGlobalPalette") || !strcmp(*argv, "#GroupPalette")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->is_global_palette = true;
                g->mode = MODE_C;
                convpng.numgroups++;
            } else
        
            if(!strcmp(*argv, "#Palette")) {
                g->palette = str_dup(argv[1]);
            } else
            
            if(!strcmp(*argv, "#PaletteMaxSize")) {
                unsigned int len = g->palette_length = (unsigned int)strtol(argv[1], NULL, 10);
                if (len > MAX_PAL_LEN) { len = MAX_PAL_LEN; }
                if (len <= 1) { errorf("invalid pallete size (line %d)", convpng.curline); }
                g->palette_fixed_length = true;
                g->palette_length = len;
            } else
            
            if(!strcmp(*argv, "#OutputPaletteImage")) {
                g->output_palette_image = true;
            } else
            
            if(!strcmp(*argv, "#OutputPaletteArray")) {
                if(!strcmp(argv[1], "false")) {
                    g->output_palette_array = false;
                } else {
                    g->output_palette_array = true;
                }
            } else
            
            if(!strcmp(*argv, "#OutputDirectory")) {
                if (argv[1][strlen(argv[1])-1] != '/') {
                    convpng.directory = str_dupcat(argv[1], "/");
                } else {
                    convpng.directory = str_dup(argv[1]);
                }
            } else
            
            if(!strcmp(*argv, "#NoPaletteArray")) {
                g->output_palette_array = false;
            } else
            
            if(!strcmp(*argv, "#Tilemap")) {
                char **tilemap_options;
        
                if(num <= 1) { args_error(); }
                num = separate_args(argv[1], &tilemap_options, ',');
                if(num < 3) { args_error(); }
                
                g->tile_width = (unsigned)strtol(tilemap_options[0], NULL, 10);
                g->tile_height = (unsigned)strtol(tilemap_options[1], NULL, 10);
                g->create_tilemap_ptrs = !strcmp(tilemap_options[2], "true");
                g->tile_size = g->tile_width * g->tile_height + 2;
                g->convert_to_tilemap = true;
                
                // free the allocated memory
                free(tilemap_options);
            } else
        
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
            } else
            
            // A C conversion type
            if (!strcmp(*argv, "#GroupC")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcatdir(argv[1], ".h");
                g->outc = str_dupcatdir(argv[1], ".c");
                g->mode = MODE_C;
                convpng.numgroups++;
            } else

            // An ASM group
            if (!strcmp(*argv, "#GroupASM")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outh = str_dupcatdir(argv[1], ".inc");
                g->outc = str_dupcatdir(argv[1], ".asm");
                g->mode = MODE_ASM;
                convpng.numgroups++;
            } else
            
            // output in ICE format
            if (!strcmp(*argv, "#GroupICE")) {
                g = &group[convpng.numgroups];
                g->name = str_dup(argv[1]);
                g->outc = str_dupcatdir(argv[1], ".txt");
                g->outh = NULL;
                g->mode = MODE_ICE;
                convpng.numgroups++;
            } else 
            
            if (!strcmp(*argv, "#PNGImages")) {
            } else
            
            errorf("unknown command %s (line %d)", *argv, convpng.curline);
            
            // free the args
            free(argv);

        } else if(*line != '/') {
            add_image(line);
        }
    }
    return num;
}
