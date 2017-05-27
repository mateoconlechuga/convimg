#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <getopt.h>

#include "libs/lodepng.h"

#include "main.h"
#include "misc.h"
#include "parser.h"
#include "logging.h"
#include "palettes.h"

// default names for ini and log files
char *ini_main_name = "convpng.ini";
char *log_main_name = "convpng.log";

static void init_convpng_struct(void) {
    unsigned t;
    convpng.curline = 0;
    convpng.numgroups = 0;
    convpng.numappvars = 0;
    for (t = 0; t < NUM_GROUPS; t++) {
        group_t *g = &group[t];
        g->palette_name = NULL;
        g->image = NULL;
        g->name = NULL;
        g->use_tcolor = false;
        g->use_tindex = false;
        g->output_palette_array = true;
        g->output_palette_image = false;
        g->is_global_palette = false;
        g->compression = COMPRESS_NONE;
        g->style = STYLE_NONE;
        g->convert_to_tilemap = false;
        g->numimages = 0;
        g->tindex = 0;
        g->mode = 0;
        g->bpp = 8;
    }
}

void init_convpng(int argc, char **argv) {
    char opt;
    char *ini_file_name = NULL;
    char *log_file_name = NULL;
    
    convpng.using_custom_ini = false;
    convpng.using_custom_log = false;
    
    // read all the options from the command line
    while ((opt = getopt(argc, argv, "c:i:j:")) != -1) {
        switch (opt) {
            case 'c':    // generate an icon header file useable with the C toolchain
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = true;
                create_icon();
				exit(0);
				break;
            case 'j':    // generate an icon for asm programs
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = false;
                create_icon();
				exit(0);
				break;
            case 'i':    // change the ini file input
                ini_file_name = safe_malloc(strlen(optarg)+5);
                strcpy(ini_file_name, optarg);
                if (!strrchr(ini_file_name, '.')) strcat(ini_file_name, ".ini");
                convpng.using_custom_ini = true;
                break;
            case 'l':    // change the log file output
                log_file_name = safe_malloc(strlen(optarg)+5);
                strcpy(log_file_name, optarg);
                if (!strrchr(log_file_name, '.')) strcat(log_file_name, ".log");
                convpng.using_custom_log = true;
                break;
            default:
                break;
        }
    }

    // check if need to change the names
    ini_file_name = ini_file_name ? ini_file_name : ini_main_name;
    log_file_name = log_file_name ? log_file_name : log_main_name;
    
    // open input and output files
    convpng.ini = fopen(ini_file_name, "r");
    convpng.log = fopen(log_file_name, "w");
    
    // ensure the files were opened correctly
    if (!convpng.ini) { errorf("could not find file '%s'\nPlease make sure you have created the configuration file\n", ini_file_name); }
    if (!convpng.log) { errorf("could not open file '%s'\nPlease check file permissions\n", log_file_name); }
    
    // init the main structure
    init_convpng_struct();
    
    // log a message that opening succeded
    lof("Opened %s\n", ini_file_name);
}

