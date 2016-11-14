#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "libs/libimagequant.h"
#include "libs/lodepng.h"
#include "libs/rle.h"
#include "libs/lz.h"

#include "main.h"
#include "misc.h"
#include "logging.h"
#include "parser.h"
#include "palettes.h"

// default names for ini and log files
char *ini_main_name = "convpng.ini";
char *log_main_name = "convpng.log";

convpng_t convpng;
group_t group[NUM_GROUPS];

int main(int argc, char **argv) {
    // init some variables
    char opt;
    char *line = NULL;
    char *ini_file_name = NULL;
    char *log_file_name = NULL;
    time_t c1 = time(NULL);
    unsigned s,g,j,k;
    
    // print out version information
    lof("ConvPNG %d.%d by M.Waltz\n\n", VERSION_MAJOR, VERSION_MINOR);
    
    // read all the options from the command line
    while ( (opt = getopt(argc, argv, "c:i:j:") ) != -1) {
        switch (opt) {
            case 'c':	// generate an icon header file useable with the C toolchain
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = true;
                return create_icon();
            case 'j':	// generate an icon for asm programs
                convpng.iconc = str_dup(optarg);
                convpng.icon_zds = false;
                return create_icon();
            case 'i':	// change the ini file input
                ini_file_name = malloc(strlen(optarg)+5);
                strcpy(ini_file_name, optarg);
                if(!strrchr(ini_file_name, '.')) strcat(ini_file_name, ".ini");
                break;
            case 'l':	// change the log file output
                log_file_name = malloc(strlen(optarg)+5);
                strcpy(log_file_name, optarg);
                if(!strrchr(log_file_name, '.')) strcat(log_file_name, ".log");
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
    if(!convpng.ini) { lof("[error] could not find file '%s'.\nPlease make sure you have created the configuration file\n", ini_file_name); exit(1); }
    if(!convpng.log) { lof("could not open file '%s'.\nPlease check file permissions\n", log_file_name); exit(1); }
    
    // init the main structure
    init_convpng_struct();
    
    // log a message that opening succeded
    lof("Opened %s\n", ini_file_name);
    
    // parse the input file
    while ((line = get_line(convpng.ini)))
        free_args(&line, &convpng.argv, parse_input(line));
    
    fclose(convpng.ini);

    // add an extra newline for kicks
    lof("\n");
    
    // convert all the groups
    for(g = 0; g < convpng.numgroups; g++) {
        // init some vars
        liq_palette pal;
        liq_image *image = NULL;
        liq_result *res = NULL;
        liq_attr *attr = NULL;
        double diff;
        unsigned inc_amt;
        uint8_t shift_amt, lob, out_byte;
         
        // get current group pointer
        group_t   *curr                = &group[g];
        
        // already inited structure elements
        uint8_t    group_bpp           = curr->bpp;
        uint16_t   group_tcolor_cv     = curr->tcolor_converted;
        unsigned   group_mode          = curr->mode;
        unsigned   group_tindex        = curr->tindex;
        unsigned   group_numimages     = curr->numimages;
        unsigned   group_tile_width    = curr->tile_width;
        unsigned   group_tile_height   = curr->tile_height;
        unsigned   group_compression   = curr->compression;
        bool       group_use_tindex    = curr->use_tindex;
        bool       group_use_tcolor    = curr->use_tcolor;
        bool       group_is_gpal       = curr->is_global_palette;
        bool       group_out_pal_img   = curr->output_palette_image;
        bool       group_out_pal_arr   = curr->output_palette_array;
        bool       group_conv_to_tiles = curr->convert_to_tilemap;
        char      *group_pal_name      = curr->palette_name;
        char      *group_name          = curr->name;
        char      *group_outc_name     = curr->outc;
        char      *group_outh_name     = curr->outh;
        unsigned   group_pal_len       = MAX_PAL_LEN;
        
        // uninited elements
        unsigned   group_tile_size       = group_tile_width * group_tile_height;
        unsigned   group_total_tile_size = group_tile_size + 2;
        FILE      *group_outc = NULL;
        FILE      *group_outh = NULL;
        
        // init new elements
        bool       is_16_bpp = group_bpp == 16;
        bool       use_transpcolor = group_use_tcolor || group_use_tindex;
        bool       group_mode_c = group_mode == MODE_C;
        bool       group_mode_asm = group_mode == MODE_ASM;
        bool       group_mode_ice = group_mode == MODE_ICE;
        
        // ICE needs specific outputs
        if(group_mode_ice) {
            group_compression = COMPRESS_NONE;
            group_conv_to_tiles = false;
            group_is_gpal = false;
            group_bpp = 8;
        }
        
        switch(group_bpp) {
            case 4:
                shift_amt = 1;
                break;
            case 2:
                shift_amt = 2;
                break;
            case 1:
                shift_amt = 3;
                break;
            default:
                shift_amt = 0;
                break;
        }
        
        // force some things if the user is an idiot
        if(is_16_bpp) {
            group_is_gpal = false;
            group_compression = COMPRESS_NONE;
            use_transpcolor = false;
            group_use_tcolor = false;
            group_use_tindex = false;
        }
        
        inc_amt = pow(2, shift_amt);
        
        // log the messages while opening the output files
        if(group_is_gpal) {
            lof("--- Global Palette %s ---\n", group_name);
        } else {
            lof("--- Group %s (%s) ---\n", group_name, group_mode_asm ? "ASM" : group_mode_c ? "C" : "ICE");
            group_outc = fopen(group_outc_name, "w");
            group_outh = fopen(group_outh_name, "w");
            if(!group_outc || !group_outh) { errorf("could not open %s.c/%s.h for output.", group_name, group_name); }
        }
        
        // check to make sure the attributes were created correctly
        if(!(attr = liq_attr_create())) { errorf("could not create image attributes."); }
        
        // set the maximum color amount
        group_pal_len = pow(2, group_bpp);
        
        // set the maximum colors of the palette
        liq_set_max_colors(attr, group_pal_len);
        
        /* check if we need a custom palette */
        if(group_pal_name) {
            uint8_t *pal_arr;
            
            // use xlibc palette
            if(!strcmp(group_pal_name, "xlibc")) {
                pal_arr = xlibc_palette;
                lof("Using built-in xlibc palette...\n");
            } else
                
            // use 332 palette
            if(!strcmp(group_pal_name, "rgb332")) {
                pal_arr = rgb332_palette;
                lof("Using built-in rgb332 palette...\n");
                
            // must be some user specified palette
            } else {
                // some variables
                uint8_t *pal_rgba;
                unsigned pal_width;
                unsigned pal_height;
                unsigned pal_size;
                
                // decode the palette
                unsigned error = lodepng_decode32_file(&pal_rgba, &pal_width, &pal_height, group_pal_name);
                if(error) { errorf("decoding palette %s", group_pal_name); }
                if(pal_height > 1 || pal_width > 256 || pal_width < 3) { errorf("palette not formatted correctly."); }
                
                // add it to the computation array
                pal_size = pal_width * pal_height;
                add_rgba(pal_rgba, pal_size << 2);
                
                // free the opened image
                free(pal_rgba);
                group_pal_len = pal_width;
                pal_arr = convpng.all_rgba;
                
                // tell the user what they are doing
                lof("Using defined palette %s...\n", group_pal_name);
            }
            
            // store the custom palette to the main palette
            unsigned h;
            liq_color pal_color;
            pal.count = group_pal_len;
            
            // loop though all the colors
            for(h = 0; h < group_pal_len; h++) {
                unsigned loc = h << 2;
                pal_color.r = pal_arr[loc + 0];
                pal_color.g = pal_arr[loc + 1];
                pal_color.b = pal_arr[loc + 2];
                pal_color.a = pal_arr[loc + 3];
                pal.entries[h] = pal_color;
            }
            
            free_rgba();
        // build the palette using maths
        } else {
            lof("Building palette with [%u] available indices ...\n", group_pal_len);
            for(s = 0; s < group_numimages; s++) {
                // some variables
                unsigned error;
                uint8_t *pal_rgba;
                unsigned pal_width;
                unsigned pal_height;
                unsigned pal_size;
                char *pal_in_name = curr->image[s]->in;
                
                // open the file
                error = lodepng_decode32_file(&pal_rgba, &pal_width, &pal_height, pal_in_name);
                if(error) { errorf("decoding %s", pal_in_name); }
                
                pal_size = pal_width * pal_height;
                add_rgba(pal_rgba, pal_size << 2);
                
                // free the opened image
                free(pal_rgba);
            }
            
            // create the rbga palette
            image = liq_image_create_rgba(attr, convpng.all_rgba, convpng.all_rgba_size >> 2, 1, 0);
            if(!image) { errorf("could not create palette."); }
            
            // add transparent color if neeeded
            if(group_use_tcolor)
                liq_image_add_fixed_color(image, curr->tcolor);
            
            // quantize the palette
            res = liq_quantize_image(attr, image);
            if(!res) {errorf("could not quantize palette."); }
            
            // get the crappiness of the user's image
            memcpy(&pal, liq_get_palette(res), sizeof(liq_palette));
            if((diff = liq_get_quantization_error(res)) > 15) { convpng.bad_conversion = true; }
            
            // get the new palette length
            group_pal_len = pal.count;
            
            // log palette conversion quality
            lof("Built palette with [%u] indices.\n", group_pal_len);
            lof("Palette quality : %.2f%%\n", 100 - diff < 0 ? 0 : 100 - diff);
            
            // find the transparent color, move by default to index 0
            if(group_use_tcolor) {
                
                // if the user wants the index to be elsewhere, expand the array
                if(group_tindex > group_pal_len) {
                    group_pal_len = group_tindex+1;
                }
                
                for(j = 0; j < group_pal_len ; j++) {
                    if(group_tcolor_cv == rgb1555(pal.entries[j].r, pal.entries[j].g, pal.entries[j].b))
                        break;
                }
            
                // move transparent color to index 0
                liq_color tmpp = pal.entries[j];
                pal.entries[j] = pal.entries[group_tindex];
                pal.entries[group_tindex] = tmpp;
            }
            
            // free all the things
            free_rgba();
        }
        
        // output an image of the palette
        if(group_out_pal_img) {
            char *png_file_name = malloc(strlen(group_name)+10);
            strcpy(png_file_name, group_name);
            strcat(png_file_name, "_pal.png");
            build_image_palette(&pal, group_pal_len, png_file_name);
            free(png_file_name);
        }
        
        // check and see if we need to build a global palette
        if(curr->is_global_palette) {
            // build an image which uses the global palette
            build_image_palette(&pal, group_pal_len, group_name);
        } else {
            // now, write the all_gfx output
            if(group_mode_c) {
                fprintf(group_outc, "// Converted using ConvPNG\n");
                fprintf(group_outc, "#include <stdint.h>\n");
                fprintf(group_outc, "#include \"%s\"\n\n", group_outh_name);
                
                if(group_out_pal_arr) {
                    fprintf(group_outc, "uint16_t %s_pal[%u] = {\n", group_name, group_pal_len);
            
                    for(j = 0; j < group_pal_len; j++) {
                        fprintf(group_outc, " 0x%04X,  // %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(pal.entries[j].r,
                                                                                                pal.entries[j].g,
                                                                                                pal.entries[j].b),j,
                                                                                                pal.entries[j].r,
                                                                                                pal.entries[j].g,
                                                                                                pal.entries[j].b,
                                                                                                pal.entries[j].a);
                    }
                    fprintf(group_outc, "};");
                }
                
                fprintf(group_outh, "// Converted using ConvPNG\n");
                fprintf(group_outh, "// This file contains all the graphics sources for easier inclusion in a project\n");
                fprintf(group_outh, "#ifndef %s_H\n#define %s_H\n", group_name, group_name);
                fprintf(group_outh, "#include <stdint.h>\n\n");
                if(use_transpcolor)
                    fprintf(group_outh, "#define %s_transpcolor_index %u\n\n", group_name, group_tindex);
                
            // write to asm output file
            } else
            if(group_mode_asm) {
                fprintf(group_outc, "; Converted using ConvPNG\n");
                fprintf(group_outc, "; This file contains all the graphics for easier inclusion in a project\n\n");
                
                if(group_out_pal_arr) {
                    fprintf(group_outc, "_%s_pal_size equ %u\n", group_name, group_pal_len*2);
                    fprintf(group_outc, "_%s_pal:\n", group_name);
                    
                    for(j = 0; j < group_pal_len; j++) {
                        fprintf(group_outc, " dw 0%04Xh ; %02u :: rgba(%u,%u,%u,%u)\n", rgb1555(pal.entries[j].r,
                                                                                                pal.entries[j].g,
                                                                                                pal.entries[j].b),j,
                                                                                                pal.entries[j].r,
                                                                                                pal.entries[j].g,
                                                                                                pal.entries[j].b,
                                                                                                pal.entries[j].a);
                    }
                }
                
                fprintf(group_outh, "; Converted using ConvPNG\n");
                fprintf(group_outh, "; This file contains all the graphics for easier inclusion in a project\n");
                fprintf(group_outh, "#ifndef %s_H\n#define %s_INC\n\n", group_name, group_name);
                fprintf(group_outh, "; ZDS sillyness\n#define db .db\n#define dw .dw\n#define dl .dl\n\n");
                fprintf(group_outh, "#include \"%s\"\n", group_outc_name);
                if(use_transpcolor)
                    fprintf(group_outh, "%s_transpcolor_index equ %u\n\n", group_name, group_tindex);
            } else
            if(group_mode_ice && group_pal_name) {
                fprintf(group_outc, "Converted using ConvPNG\n");
                fprintf(group_outc, "This file contains all the converted graphics\n\n");
                
                if(group_out_pal_arr) {
                    if(use_transpcolor)
                        fprintf(group_outc, "%s_transpcolor_index | %u\n\n", group_name, group_tindex);
                    fprintf(group_outc, "%s_pal | %u bytes\n\"", group_name, group_pal_len*2);
                    
                    for(j = 0; j < group_pal_len; j++) {
                        fprintf(group_outc, "%04X", rgb1555(pal.entries[j].r, pal.entries[j].g, pal.entries[j].b));
                    }
                    
                    fprintf(group_outc, "\"\n\n");
                }
            }
            
            // log transparent color things
            if(use_transpcolor) {
                lof("TranspColorIndex : %u\n", group_tindex);
                lof("TranspColor : 0x%04X\n", group_tcolor_cv);
            }

            // log the number of images we are converting
            lof("%d:\n", group_numimages);
            
            // okay, now we have the palette used for all the images. Let's fix them to the attributes
            for(s = 0; s < group_numimages; s++) {
                // init some vars
                unsigned error;
                uint8_t *image_data = NULL;
                unsigned char *image_rgba = NULL;
                
                liq_image *image_image = NULL;
                liq_result *image_res = NULL;
                liq_attr *image_attr = liq_attr_create();
                
                FILE *image_outc = NULL;
                
                // get the address of the current working image
                image_t *curr_image = curr->image[s];
                
                // init the things for each image
                char       *image_outc_name      = curr_image->outc;
                char       *image_in_name        = curr_image->in;
                char       *image_name           = curr_image->name;
                unsigned    image_width;
                unsigned    image_height;
                unsigned    image_size;
                unsigned    total_image_size;
                unsigned    image_total_tiles    = 0;
                
                // open the file and make a rgba array
                error = lodepng_decode32_file(&image_rgba, &image_width, &image_height, image_in_name);
                if(error) { errorf("decoding %s", image_in_name); }
                
                // get the actual size of the image
                image_size = image_width * image_height;
                total_image_size = image_size + 2;

                // quick tilemap check
                if(group_conv_to_tiles) {
                    image_total_tiles = (image_width / group_tile_width) * (image_height / group_tile_height);
                    
                    if((image_width % group_tile_width) || (image_height % group_tile_height)) {
                        errorf("image dimensions do not align to tile width and/or height values.");
                    }
                }
                
                // allocate and decode the image
                image_data = malloc(image_size + 2);
                liq_set_max_colors(image_attr, group_pal_len);
                image_image = liq_image_create_rgba(image_attr, image_rgba, image_width, image_height, 0);
                if(!image_image) { errorf("could not create image."); }
                
                // add all the palette colors
                for(j = 0; j < group_pal_len; j++) { liq_image_add_fixed_color(image_image, pal.entries[j]); }
                
                // quantize image against palette
                if(!(image_res = liq_quantize_image(image_attr, image_image))) {errorf("could not quantize image."); }
                liq_write_remapped_image(image_res, image_image, image_data, image_size);
                
                // if custom palette, hard to compute accuratly
                if(group_pal_name) {
                    lof(" %s : converted!", image_name);
                    convpng.bad_conversion = true;
                } else {
                    if((diff = liq_get_remapping_error(image_res)) > 15) { convpng.bad_conversion = true; }
                    lof(" %s : %.2f%%", image_name, 100-diff > 0 ? 100-diff : 0);
                }
                
                // open the outputs
                if(!group_mode_ice) {
                    if(!(image_outc = fopen(image_outc_name, "w"))) { errorf("opening file for output."); }
                }
                
                // write all the image data to the ouputs
                if(group_mode_c) {
                    fprintf(image_outc, "// Converted using ConvPNG\n");
                    fprintf(image_outc, "#include <stdint.h>\n#include \"%s\"\n\n", group_outh_name);
                } else
                if(group_mode_asm) {
                    fprintf(image_outc, "; Converted using ConvPNG\n\n");
                    if(!group_conv_to_tiles) {
                            fprintf(image_outc, "_%s_width equ %u\n", image_name, image_width);
                            fprintf(image_outc, "_%s_height equ %u\n", image_name, image_height);
                    }
                }
                
                // using some kind of compression?
                if(group_compression) {
                    unsigned compressed_size = 0;
                    unsigned total_compressed_size = 0;
                    uint8_t compressed_size_high, compressed_size_low;
                    
                    if(!group_conv_to_tiles) {
                        // allocate the datas
                        uint8_t *orig_data = malloc(total_image_size);
                        uint8_t *compressed_data = malloc((total_image_size << 1) + 1);

                        // store the width and height as well
                        orig_data[0] = image_width;
                        orig_data[1] = image_height;
                        
                        // copy in the image data
                        memcpy(&orig_data[2], image_data, image_size);
                        
                        // compress the data
                        switch(group_compression) {
                            case COMPRESS_LZ:
                                compressed_size = (unsigned)LZ_Compress(orig_data, compressed_data, total_image_size);
                                break;
                            case COMPRESS_RLE:
                                compressed_size = (unsigned)RLE_Compress(orig_data, compressed_data, total_image_size);
                                break;
                            default:
                                break;
                        }
                        
                        total_compressed_size = compressed_size + 2;
                        compressed_size_low = compressed_size & 255;
                        compressed_size_high = (compressed_size >> 8) & 255;
                        
                        // ouput the compressed data array start
                        if(group_mode_c) {
                            fprintf(image_outc, "uint8_t %s_data_compressed[%u] = {\n 0x%02X,0x%02X, // compressed size\n ", image_name,
                                                                                                                             total_compressed_size,
                                                                                                                             compressed_size_low,
                                                                                                                             compressed_size_high);
                        } else
                        if(group_mode_asm) {
                            fprintf(image_outc, "_%s_data_compressed_size equ %u\n", image_name, total_compressed_size);
                            fprintf(image_outc, "_%s_data_compressed:\n db 0%02Xh,0%02Xh ; compressed size\n db ", image_name, compressed_size_low, compressed_size_high);
                        }
                        
                        // output the compressed data array
                        output_compressed_array(image_outc, compressed_data, compressed_size, group_mode);
                        
                        // log the compression ratios
                        lof(" (compress: %u -> %d bytes)\n", total_image_size, total_compressed_size);
                        
                        // if compression just makes a bigger image, say something about it
                        if(total_compressed_size > total_image_size) { lof(" #warning!"); }
                        total_image_size = total_compressed_size;
                        
                        // free the temporary arrays
                        free(orig_data);
                        free(compressed_data);
                    } else {
                        unsigned curr_tile;
                        unsigned x_offset, y_offset, offset, index;
                        uint8_t *orig_data = malloc(total_image_size);
                        uint8_t *compressed_data = malloc((total_image_size << 1) + 1);
                        
                        for(x_offset = y_offset = curr_tile = 0; curr_tile < image_total_tiles; curr_tile++) {
                            orig_data[0] = group_tile_width;
                            orig_data[1] = group_tile_height;
                            index = 2;
                            
                            // convert a single tile
                            for(j = 0; j < group_tile_height; j++) {
                                offset = j * image_width;
                                for(k = 0; k < group_tile_width; k++) {
                                    orig_data[index++] = image_data[k + x_offset + y_offset + offset];
                                }
                            }
                            
                            // select the compression mode
                            switch(group_compression) {
                                case COMPRESS_LZ:
                                    compressed_size = (unsigned)LZ_Compress(orig_data, compressed_data, group_total_tile_size);
                                    break;
                                case COMPRESS_RLE:
                                    compressed_size = (unsigned)RLE_Compress(orig_data, compressed_data, group_total_tile_size);
                                    break;
                                default:
                                    errorf("unexpected compression mode.");
                                    break;
                            }
                            
                            total_compressed_size = compressed_size + 2;
                            compressed_size_low = compressed_size & 255;
                            compressed_size_high = (compressed_size >> 8) & 255;
                        
                            // output to c or asm file
                            if(group_mode_c) {
                                fprintf(image_outc, "uint8_t %s_tile_%u_data_compressed[%u] = {\n 0x%02X,0x%02X, // compressed size\n ", image_name,
                                                                                                                                         curr_tile,
                                                                                                                                         total_compressed_size,
                                                                                                                                         compressed_size_low,
                                                                                                                                         compressed_size_high);
                            } else
                            if(group_mode_asm) {
                                fprintf(image_outc, "_%s_tile_%u_size equ %u\n", image_name, curr_tile, total_compressed_size);
                                fprintf(image_outc, "_%s_tile_%u_compressed:\n db 0%02Xh,0%02Xh ; compressed size\n db ", image_name, 
                                                                                                                          curr_tile,
                                                                                                                          compressed_size_low,
                                                                                                                          compressed_size_high);
                            }
                            
                            // output the compressed data array
                            output_compressed_array(image_outc, compressed_data, compressed_size, group_mode);
                            
                            // log the new size of the tile
                            lof("\n %s_tile_%u_compressed (compress: %u -> %d bytes) (%s)", image_name,
                                                                                            curr_tile,
                                                                                            group_total_tile_size,
                                                                                            total_compressed_size,
                                                                                            image_outc_name);
                            
                            // if compression just makes a bigger image, say something about it
                            if(total_compressed_size > group_total_tile_size) { lof(" #warning!"); }
                            
                            // move to the correct data location
                            if((x_offset += group_tile_width) > image_width - 1) {
                                x_offset = 0; y_offset += image_width * group_tile_height;
                            }
                        }
                        
                        // build the tilemap table
                        if(curr->create_tilemap_ptrs) {
                            if(group_mode_c) {
                                fprintf(image_outc, "uint8_t *%s_tiles_data_compressed[%u] = {\n", image_name, image_total_tiles);
                                for(curr_tile = 0; curr_tile < image_total_tiles; curr_tile++)
                                    fprintf(image_outc, " %s_tile_%u_data_compressed,\n", image_name, curr_tile);
                                fprintf(image_outc, "};\n");
                            } else {
                                fprintf(image_outc, "_%s_tiles_compressed: ; %u tiles\n", image_name, image_total_tiles);
                                for(curr_tile = 0; curr_tile < image_total_tiles; curr_tile++)
                                    fprintf(image_outc, " dl _%s_tile_%u_compressed\n", image_name, curr_tile);
                            }
                        }
                        
                        // free the datas
                        free(orig_data);
                        free(compressed_data);
                    }
                } else { // no compression
                    unsigned curr_tile;
                    unsigned x_offset, y_offset, offset;
                    
                    x_offset = y_offset = 0;
                    
                    lof(" (%s)\n", image_outc_name);
                    
                    if(group_conv_to_tiles) {
                        // convert the file with the tilemap formatting
                        for(curr_tile = 0; curr_tile < image_total_tiles; curr_tile++) {
                            if(group_mode_c) {
                                fprintf(image_outc, "uint8_t %s_tile_%u_data[%u] = {\n %u,\t// tile_width\n %u,\t// tile_height\n ", image_name,
                                                                                                                                     curr_tile,
                                                                                                                                     group_tile_size + 2,
                                                                                                                                     group_tile_width,
                                                                                                                                     group_tile_height);
                            } else
                            if(group_mode_asm) {
                                fprintf(image_outc, "_%s_tile_%u: ; %u bytes\n db %u,%u ; width,height\n db ", image_name,
                                                                                                               curr_tile,
                                                                                                               group_tile_size + 2,
                                                                                                               group_tile_width,
                                                                                                               group_tile_height);
                            }
                            
                            // convert a single tile
                            for(j = 0; j < group_tile_height; j++) {
                                offset = j * image_width;
                                for(k = 0; k < group_tile_width; k++) {
                                    if(group_mode_c) {
                                        fprintf(image_outc, "0x%02X%c", image_data[k + x_offset + y_offset + offset], ',');
                                    } else
                                    if(group_mode_asm) {
                                        fprintf(image_outc, "0%02Xh%c", image_data[k + x_offset + y_offset + offset], k+1 == group_tile_width ? ' ' : ',');
                                    }
                                }
                                // check if at the end
                                if(group_mode_c) {
                                    fprintf(image_outc, "\n%s", j+1 != group_tile_height ? " " : "};\n\n");
                                } else
                                if(group_mode_asm) {
                                    fprintf(image_outc, "\n%s", j+1 != group_tile_height ? " db " : "\n\n");
                                }
                            }
                            
                            // move to the correct data location
                            if((x_offset += group_tile_width) > image_width - 1) {
                                x_offset = 0;
                                y_offset += image_width * group_tile_height;
                            }
                        }
                        
                        // build the tilemap table
                        if(curr->create_tilemap_ptrs) {
                            if(group_mode_c) {
                                fprintf(image_outc, "uint8_t *%s_tiles_data[%u] = {\n", image_name, image_total_tiles);
                                for(curr_tile = 0; curr_tile < image_total_tiles; curr_tile++) {
                                    fprintf(image_outc, " %s_tile_%u_data,\n", image_name, curr_tile);
                                }
                                fprintf(image_outc, "};\n");
                            } else {
                                fprintf(image_outc, "_%s_tiles: ; %u tiles\n", image_name, image_total_tiles);
                                for(curr_tile = 0; curr_tile < image_total_tiles; curr_tile++) {
                                    fprintf(image_outc, " dl _%s_tile_%u\n", image_name, curr_tile);
                                }
                            }
                        }
                    
                    // not a tilemap, just a normal image
                    } else {
                        if(group_mode_c) {
                            fprintf(image_outc, "// %u bpp image\nuint8_t %s_data[%u] = {\n %u,%u,  // width,height\n ", group_bpp,
                                                                                                                         image_name,
                                                                                                                         total_image_size,
                                                                                                                         image_width,
                                                                                                                         image_height);
                        } else
                        if(group_mode_asm) {
                            fprintf(image_outc, "_%s: ; %u bytes\n db %u,%u\n db ", image_name, total_image_size, image_width, image_height);
                        } else
                        if(group_mode_ice) {
                            fprintf(group_outc, "%s | %u bytes\n%u,%u,\"", image_name, total_image_size, image_width, image_height);
                        }
                        
                        // output the data to the file
                        if(!is_16_bpp) {
                            uint8_t curr_inc;
                            for(j = 0; j < image_height; j++) {
                                int offset = j * image_width;
                                for(k = 0; k < image_width; k += inc_amt) {
                                    for(curr_inc = inc_amt, out_byte = lob = 0; lob < inc_amt; lob++)
                                        out_byte |= (image_data[k + offset + lob] << --curr_inc);
                                    if(group_mode_c)
                                        fprintf(image_outc,"0x%02X,", out_byte);
                                    else
                                    if(group_mode_asm)
                                        fprintf(image_outc,"0%02Xh%c", out_byte, k + inc_amt == image_width ? ' ' : ',');
                                    else
                                    if(group_mode_ice)
                                        fprintf(group_outc,"%02X", out_byte);
                                }
                                if(!group_mode_ice)
                                    fprintf(image_outc, "\n%s", (group_mode_c) ? ((j+1 != image_height) ? " " : "};\n") 
                                                                               : ((j+1 != image_height) ? " db " : "\n"));
                            }
                            if(group_mode_ice) {
                                fprintf(group_outc, "\"\n\n");
                            }
                        } else { // 16 bpp
                            uint8_t out_byte_high, out_byte_low;
                            for(j = 0; j < image_height; j++) {
                                int offset = j * image_width;
                                for(k = 0; k < image_width; k++) {
                                    uint16_t out_short = rgb565(image_rgba[offset + k + 0], 
                                                                image_rgba[offset + k + 1],
                                                                image_rgba[offset + k + 2]);
                                    out_byte_high = out_short >> 8;
                                    out_byte_low = out_short & 255;
                                    if(group_mode_c)
                                        fprintf(image_outc, "0x%02X,0x%02X,", out_byte_high, out_byte_low);
                                    else
                                        fprintf(image_outc, "0%02Xh,0%02Xh%c", out_byte_high, out_byte_low, k + 1 == image_width ? ' ' : ',');
                                }
                                fprintf(image_outc, "\n%s", (group_mode_c) ? ((j+1 != image_height) ? " " : "};\n") 
                                                                           : ((j+1 != image_height) ? " db " : "\n"));
                            }
                        }
                    }
                }
                
                if(group_conv_to_tiles) {
                    unsigned curr_tile;
                    if(group_mode_c) {
                        for(curr_tile = 0; curr_tile < image_total_tiles; curr_tile++) {
                            if(group_compression == COMPRESS_NONE) {
                                fprintf(group_outh,"extern uint8_t %s_tile_%u_data[%u];\n", image_name, curr_tile, group_tile_size + 2);
                                fprintf(group_outh, "#define %s_tile_%u ((gfx_image_t*)%s_tile_%u_data)\n", image_name, curr_tile, image_name, curr_tile);
                            } else {
                                fprintf(group_outh,"extern uint8_t %s_tile_%u_data_compressed[];\n", image_name, curr_tile);
                            }
                        }

                        if(curr->create_tilemap_ptrs) {
                            if(group_compression == COMPRESS_NONE) {
                                fprintf(group_outh, "extern uint8_t *%s_tiles_data[%u];\n", image_name, image_total_tiles);
                                fprintf(group_outh, "#define %s_tiles ((gfx_image_t**)%s_tiles_data)\n", image_name, image_name);
                            } else {
                                fprintf(group_outh, "extern uint8_t *%s_tiles_data_compressed[%u];\n", image_name, image_total_tiles);
                            }
                        }
                    } else { // asm output
                        fprintf(group_outh, "#include \"%s\"\n", image_outc_name);
                    }
                } else { // not a tilemap
                    if(group_mode_c) {
                        if(group_compression == COMPRESS_NONE) {
                            fprintf(group_outh, "extern uint8_t %s_data[%u];\n", image_name, total_image_size);
                            fprintf(group_outh, "#define %s ((gfx_image_t*)%s_data)\n", image_name, image_name);
                        } else { // some kind of compression
                            fprintf(group_outh, "extern uint8_t %s_data_compressed[%u];\n", image_name, total_image_size);
                        }
                    } else
                    if(group_mode_asm) { // asm output
                        fprintf(group_outh, "#include \"%s\" ; %u bytes\n", image_outc_name, total_image_size);
                    }
                }
                
                // close the outputs
                fclose(image_outc);
                
                // free the opened image
                free(image_rgba);
                free(image_data);
                free(image_name);
                free(image_in_name);
                free(image_outc_name);
                liq_result_destroy(image_res);
                liq_image_destroy(image_image);
                liq_attr_destroy(image_attr);
            }
            
            if(group_out_pal_arr) {
                if(group_mode_c) {
                    fprintf(group_outh, "extern uint16_t %s_pal[%u];\n", group_name, group_pal_len);
                }
            }
            if(!group_mode_ice)
                fprintf(group_outh, "\n#endif\n");
            fclose(group_outh);
            fclose(group_outc);
        }
        
        // free *everything*
        free(group_name);
        free(group_outc_name);
        free(group_outh_name);
        free(group_pal_name);
        liq_attr_destroy(attr);
        liq_image_destroy(image);
        lof("\n");
    }
    
    lof("Converted in %u s\n\n", (unsigned)(time(NULL)-c1));
    if(convpng.bad_conversion) {
        lof("[warning] image quality might be too low.\nplease try grouping similar images, reducing image colors, \nor selecting a better palette if conversion is not ideal.\n\n");
    }
    lof("Finished!\n");
    
    return 0;
}

void init_convpng_struct(void) {
    unsigned t;
    convpng.curline = 0;
    convpng.all_rgba_size = 0;
    convpng.numgroups = 0;
    convpng.all_rgba = NULL;
    for(t = 0; t < NUM_GROUPS; t++) {
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
        g->convert_to_tilemap = false;
        g->numimages = 0;
        g->tindex = 0;
        g->mode = 0;
        g->bpp = 8;
    }
}

// add an rgba color to the main palette
void add_rgba(uint8_t *pal, size_t size) {
    convpng.all_rgba = realloc(convpng.all_rgba, convpng.all_rgba_size + size);
    if(!convpng.all_rgba) { errorf("memory allocation."); }
    memcpy(&convpng.all_rgba[convpng.all_rgba_size], pal, size);
    convpng.all_rgba_size += size;
}

// free the massive array
void free_rgba(void) {
    free(convpng.all_rgba);
    convpng.all_rgba = NULL;
    convpng.all_rgba_size = 0;
}

void output_compressed_array(FILE *outfile, uint8_t *compressed_data, unsigned len, unsigned mode) {
    unsigned j, k;
    
    /* write the whole array in a big block */
    for(k = j = 0; j < len; j++, k++) {
        if(mode == MODE_C) {
            fprintf(outfile, "0x%02X,", compressed_data[j]);
        } else
        if(mode == MODE_ASM) {
            fprintf(outfile, "0%02Xh%c", compressed_data[j], j+1 == len || (k+1) & 32 ? ' ' : ',');
        }
        
        if((k+1) & 32 && j+1 < len) {
            k = -1;
            if(mode == MODE_C) {
                fprintf(outfile, "\n ");
            } else
            if(mode == MODE_ASM) {
                fprintf(outfile, "\n db ");
            }
        }
    }
    if(mode == MODE_C) { fprintf(outfile, "\n};\n"); }
}
