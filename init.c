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
#include "appvar.h"
#include "parser.h"
#include "logging.h"
#include "palettes.h"

// default names for ini and log files
char *ini_main_name = "convpng.ini";
char *log_main_name = "convpng.log";

static void init_convpng_struct(void) {
    unsigned int t;
    convpng.curline = 0;
    convpng.numgroups = 0;
    convpng.numappvars = 0;
    convpng.directory = NULL;
    convpng.log = NULL;
    convpng.ini = NULL;
    convpng.using_custom_ini = false;
    convpng.using_custom_log = false;
    for (t = 0; t < NUM_GROUPS; t++) {
        group_t *g = &group[t];
        g->palette = NULL;
        g->palette_length = MAX_PAL_LEN;
        g->image = NULL;
        g->name = NULL;
        g->use_tcolor = false;
        g->use_tindex = false;
        g->output_palette_array = true;
        g->output_palette_image = false;
        g->output_palette_appvar = false;
        g->is_global_palette = false;
        g->compression = COMPRESS_NONE;
        g->style = STYLE_NONE;
        g->convert_to_tilemap = false;
        g->palette_fixed_length = false;
        g->numimages = 0;
        g->tindex = 0;
        g->mode = 0;
        g->bpp = 8;
    }
    for (t = 0; t < MAX_APPVARS; t++) {
        appvar_t *a = &appvar[t];
        a->write_init = true;
        a->mode = 0;
        a->add_offset = true;
        a->palette = NULL;
        a->palette_data = NULL;
    }
}

void init_convpng(int argc, char **argv) {
    char opt;
    char *ini_file_name = NULL;
    char *log_file_name = NULL;
    bool use_file_log = true;
    
    // init the main structure
    init_convpng_struct();
    
    // init the gamma lut
    liq_init_gamma_lut();
    
    // read all the options from the command line
    while ((opt = getopt(argc, argv, "nc:i:j:")) != -1) {
        switch (opt) {
            case 'c':    // generate an icon header file useable with the C toolchain
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = true;
                create_icon();
                if(convpng.log) { fclose(convpng.log); }
                exit(0);
                break;
            case 'j':    // generate an icon for asm programs
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = false;
                create_icon();
                if(convpng.log) { fclose(convpng.log); }
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
            case 'n':    // turn off file logging
                use_file_log = false;
                break;
            default:
                break;
        }
    }

    // check if need to change the names
    ini_file_name = ini_file_name ? ini_file_name : ini_main_name;
    log_file_name = log_file_name ? log_file_name : log_main_name;
    
    // open input and output files
    convpng.ini = fopen(ini_file_name, "rb");
    if(use_file_log) { convpng.log = fopen(log_file_name, "wb"); }
    
    // ensure the files were opened correctly
    if (!convpng.ini) { errorf("could not find file '%s'\nPlease make sure you have created the configuration file\n", ini_file_name); }
    if (!convpng.log && use_file_log) { errorf("could not open file '%s'\nPlease check file permissions\n", log_file_name); }
    
    // log a message that opening succeded
    lof("Opened %s\n", ini_file_name);
}

int cleanup_convpng(void) {
    if (convpng.log) {
        fclose(convpng.log);
    }
    free(convpng.directory);
    return 0;
}
