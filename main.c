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

#include "init.h"
#include "main.h"
#include "misc.h"
#include "parser.h"
#include "format.h"
#include "appvar.h"
#include "logging.h"
#include "palettes.h"

convpng_t convpng;
group_t group[NUM_GROUPS];
                        
int main(int argc, char **argv) {
    char *line = NULL;
    unsigned int s,g,j,k;
    time_t c1 = time(NULL);
    
    // print out version information
    lof("ConvPNG %d.%d by M.Waltz\n\n", VERSION_MAJOR, VERSION_MINOR);
    
    // init the system
    init_convpng(argc, argv);
    
    // parse the input file
    while ((line = get_line(convpng.ini))) {
        parse_input(line);
        free(line);
    }

    // close the ini file
    fclose(convpng.ini);

    // add an extra newline for kicks
    lof("\n");

    // convert all the groups
    for (g = 0; g < convpng.numgroups; g++) {
        // init some vars
        liq_palette pal;
        liq_image *image = NULL;
        liq_attr *attr = NULL;
        const format_t *format = NULL;
        output_t *g_output = output_create();
        double diff;
        
        // get current group pointer
        group_t   *curr                = &group[g];
        unsigned   g_mode              = curr->mode;
        
        // already inited structure elements
        uint8_t    g_bpp               = curr->bpp;
        uint16_t   g_tcolor_cv         = curr->tcolor_converted;
        unsigned   g_tindex            = curr->tindex;
        unsigned   g_numimages         = curr->numimages;
        unsigned   g_tile_width        = curr->tile_width;
        unsigned   g_tile_height       = curr->tile_height;
        unsigned   g_compression       = curr->compression;
        bool       g_use_tindex        = curr->use_tindex;
        bool       g_use_tcolor        = curr->use_tcolor;
        bool       g_is_global_pal     = curr->is_global_palette;
        bool       g_out_pal_img       = curr->output_palette_image;
        bool       g_out_pal_arr       = curr->output_palette_array;
        bool       g_convert_to_tiles  = curr->convert_to_tilemap;
        bool       g_make_tilemap_ptrs = curr->create_tilemap_ptrs;
        char      *g_pal_name          = curr->palette_name;
        char      *g_name              = curr->name;
        char      *g_outc_name         = curr->outc;
        char      *g_outh_name         = curr->outh;
        unsigned   g_pal_len           = MAX_PAL_LEN;
        
        // init new elements
        bool       g_is_16_bpp         = g_bpp == 16;
        bool       g_mode_c            = g_mode == MODE_C;
        bool       g_mode_asm          = g_mode == MODE_ASM;
        bool       g_mode_ice          = g_mode == MODE_ICE;
        bool       g_use_transpcolor   = g_use_tcolor || g_use_tindex;
        bool       g_style_tp          = curr->style == STYLE_TRANSPARENT;
        
        // determine the output format
        if (g_mode_c) {
            format = &c_format;
        } else
        if (g_mode_asm) {
            format = &asm_format;
        } else
        if (g_mode_ice) {
            format = &ice_format;
        }
        
        if (g_style_tp) {
            if (g_bpp != 8) {
                errorf("unsupported bpp for current style");
            }
        }
        
        // force inputs if 16 bpp image group
        if (g_is_16_bpp) {
            free(g_pal_name);
            g_pal_name        = NULL;
            g_is_global_pal   = false;
            g_use_transpcolor = false;
            g_use_tcolor      = false;
            g_use_tindex      = false;
            g_out_pal_arr     = false;
        }
        
        // log the messages while opening the output files
        if (g_is_global_pal) {
            lof("--- Global Palette %s ---\n", g_name);
        } else {
            lof("--- Group %s (%s) ---\n", g_name, g_mode_asm ? "ASM" : g_mode_c ? "C" : "ICE");
            format->open_output(g_output, g_outc_name, OUTPUT_SOURCE);
            format->open_output(g_output, g_outh_name, OUTPUT_HEADER);
        }
        
        // check to make sure the attributes were created correctly
        if (!(attr = liq_attr_create())) { errorf("could not create image attributes."); }
        
        // set the maximum color amount
        g_pal_len = pow(2, g_bpp);
        
        // set the maximum colors of the palette
        liq_set_max_colors(attr, g_pal_len);
        
        /* check if we need a custom palette */
        if (g_pal_name) {
            uint8_t *pal_arr = NULL;
            uint8_t *custom_pal = NULL;
            
            // use xlibc palette
            if (!strcmp(g_pal_name, "xlibc")) {
                pal_arr = xlibc_palette;
                lof("Using built-in xlibc palette...\n");
            } else
                
            // use 332 palette
            if (!strcmp(g_pal_name, "rgb332")) {
                pal_arr = rgb332_palette;
                lof("Using built-in rgb332 palette...\n");
                
            // must be some user specified palette
            } else {
                // some variables
                unsigned int pal_height;
                
                // decode the palette
                unsigned int error = lodepng_decode32_file(&custom_pal, &g_pal_len, &pal_height, g_pal_name);
                if (error) { errorf("decoding palette %s", g_pal_name); }
                if (pal_height > 1 || g_pal_len > 256 || g_pal_len < 3) { errorf("palette not formatted correctly."); }
                
                // we should use the custom palette
                pal_arr = custom_pal;
                
                // tell the user what they are doing
                lof("Using defined palette %s...\n", g_pal_name);
            }
            
            // store the custom palette to the main palette
            liq_color pal_color;
            pal.count = g_pal_len;
            
            // loop though all the colors
            for (unsigned int h = 0; h < g_pal_len; h++) {
                unsigned int loc = h * 4;
                pal_color.r = pal_arr[loc + 0];
                pal_color.g = pal_arr[loc + 1];
                pal_color.b = pal_arr[loc + 2];
                pal_color.a = pal_arr[loc + 3];
                pal.entries[h] = pal_color;
            }
            
            // if we allocated a custom palette free it
            free(custom_pal);
        // build the palette using maths
        } else if (!g_is_16_bpp) {
            liq_result *res = NULL;
            liq_histogram *hist = liq_histogram_create(attr);
            lof("Building palette with [%u] available indices ...\n", g_pal_len);
            for (s = 0; s < g_numimages; s++) {
                // some variables
                unsigned error;
                uint8_t *pal_rgba;
                unsigned pal_width;
                unsigned pal_height;
                liq_image *pal_image;
                char *pal_in_name = curr->image[s]->in;

                // open the file
                error = lodepng_decode32_file(&pal_rgba, &pal_width, &pal_height, pal_in_name);
                if (error) { errorf("decoding %s for palette.", pal_in_name); }
                
                pal_image = liq_image_create_rgba(attr, pal_rgba, pal_width, pal_height, 0);
                liq_histogram_add_image(hist, attr, pal_image);
                liq_image_destroy(pal_image);

                // free the opened image
                free(pal_rgba);
            }
            
            // add transparent color if neeeded
            if (g_use_tcolor) {
                liq_histogram_entry hist_entry;
                hist_entry.color = curr->tcolor;
                liq_error err = liq_histogram_add_colors(hist, attr, &hist_entry, hist_entry.count = 1, 0);
                if (err != LIQ_OK) { errorf("adding transparent color to palette."); }
            }
            
            liq_error err = liq_histogram_quantize(hist, attr, &res);
            if (err != LIQ_OK) { errorf("generating quantized palette."); }
            
            // get the crappiness of the user's image
            memcpy(&pal, liq_get_palette(res), sizeof(liq_palette));
            if ((diff = liq_get_quantization_error(res)) > 15) { convpng.bad_conversion = true; }
            
            // get the new palette length
            g_pal_len = pal.count;
            
            // log palette conversion quality
            lof("Built palette with [%u] indices.\n", g_pal_len);
            lof("Palette quality : %.2f%%\n", 100 - diff < 0 ? 0 : 100 - diff);
            
            // find the transparent color, move by default to index 0
            if (g_use_tcolor) {
                
                // if the user wants the index to be elsewhere, expand the array
                if (g_tindex > g_pal_len) {
                    g_pal_len = g_tindex+1;
                }
                
                for (j = 0; j < g_pal_len ; j++) {
                    liq_color *c = &pal.entries[j];
                    if (g_tcolor_cv == rgb1555(c->r, c->g, c->b))
                        break;
                }
                
                // move transparent color to index 0
                liq_color tmpc = pal.entries[j];
                pal.entries[j] = pal.entries[g_tindex];
                pal.entries[g_tindex] = tmpc;
            }
            
            // free the histogram
            if (res)  { liq_result_destroy(res);     }
            if (hist) { liq_histogram_destroy(hist); }
        }

        // output an image of the palette
        if (g_out_pal_img) {
            char *png_file_name = safe_malloc(strlen(g_name)+10);
            strcpy(png_file_name, g_name);
            strcat(png_file_name, "_pal.png");
            build_image_palette(&pal, g_pal_len, png_file_name);
            free(png_file_name);
        }
        
        // check and see if we need to build a global palette
        if (curr->is_global_palette) {
            // build an image which uses the global palette
            build_image_palette(&pal, g_pal_len, g_name);
        } else {
            format->print_source_header(g_output, g_outh_name);
            format->print_header_header(g_output, g_name);
            
            if (g_out_pal_arr) {
                format->print_palette(g_output, g_name, &pal, g_pal_len);
            }
            
            // log transparent color things
            if (g_use_transpcolor) {
                format->print_transparent_index(g_output, g_name, g_tindex);
                lof("Transparent Color Index : %u\n", g_tindex);
                lof("Transparent Color : 0x%04X\n", g_tcolor_cv);
            }
            
            // log the number of images we are converting
            lof("%d:\n", g_numimages);
            
            // okay, now we have the palette used for all the images. Let's fix them to the attributes
            for (s = 0; s < g_numimages; s++) {
                // get the address of the current working image
                image_t *i_curr             = curr->image[s];
                
                // init some vars
                uint8_t      *i_data        = NULL;
                uint8_t      *i_rgba        = NULL;
                
                liq_image    *i_image       = NULL;
                liq_result   *i_mapped      = NULL;
                liq_attr     *i_attr        = liq_attr_create();
                
                // init the things for each image
                char         *i_source_name = i_curr->outc;
                char         *i_in_name     = i_curr->in;
                char         *i_name        = i_curr->name;
                uint8_t       i_bpp         = g_bpp;
                uint8_t       i_tindex      = g_tindex;
                unsigned int  i_tile_width  = g_tile_width;
                unsigned int  i_tile_height = g_tile_height;
                unsigned int  i_num_tiles   = 0;
                unsigned int  i_width;
                unsigned int  i_height;
                unsigned int  i_size;
                unsigned int  i_error;
                
                // open the file and make a rgba array
                i_error = lodepng_decode32_file(&i_rgba, &i_width, &i_height, i_in_name);
                if (i_error) { errorf("decoding %s", i_in_name); }
                
                // get the actual size of the image
                i_size = i_width * i_height;

                // quick tilemap check
                if (g_convert_to_tiles) {
                    i_num_tiles = (i_width / g_tile_width) * (i_height / g_tile_height);
                    
                    if ((i_width % g_tile_width) || (i_height % g_tile_height)) {
                        errorf("image dimensions do not align to tile width and/or height value");
                    }
                }
                
                // allocate and decode the image
                i_data = safe_malloc(i_size + 2);
                liq_set_max_colors(i_attr, g_pal_len);
                i_image = liq_image_create_rgba(i_attr, i_rgba, i_width, i_height, 0);
                if (!i_image) { errorf("could not create image."); }

                // add all the palette colors
                for (j = 0; j < g_pal_len; j++) { liq_image_add_fixed_color(i_image, pal.entries[j]); }

                // quantize image against palette
                if (!(i_mapped = liq_quantize_image(i_attr, i_image))) {errorf("could not quantize image."); }
                liq_write_remapped_image(i_mapped, i_image, i_data, i_size);

                // if custom palette, hard to compute accuratly
                if (g_pal_name) {
                    lof(" %s : converted!", i_name);
                    convpng.bad_conversion = true;
                } else {
                    if ((diff = liq_get_remapping_error(i_mapped)) > 15) { convpng.bad_conversion = true; }
                    lof(" %s : %.2f%%", i_name, 100-diff > 0 ? 100-diff : 0);
                }
                
                // open the outputs
                output_t *i_output;
                
                if (!g_mode_ice) {
                    i_output = output_create();
                    format->open_output(i_output, i_source_name, OUTPUT_SOURCE);
                } else {
                    i_output = g_output;
                }
                
                // free the source name
                free(i_source_name);
                
                // write all the image data to the ouputs
                format->print_image_source_header(i_output, g_outh_name);
                
                uint8_t *i_data_buffer = safe_malloc(i_width * i_height * 2);
                unsigned int i_size_total = i_size + 2;
                unsigned int i_size_backup = i_size + 2;
                
                if (g_convert_to_tiles) {
                    unsigned int curr_tile;
                    unsigned int x_offset;
                    unsigned int y_offset;
                    unsigned int offset;
                    unsigned int index;
                    i_size        = i_tile_width * i_tile_height;
                    i_size_total  = i_size + 2;
                    i_size_backup = i_size_total;
                    
                    for (x_offset = y_offset = curr_tile = 0; curr_tile < i_num_tiles; curr_tile++) {
                        i_data_buffer[0] = i_tile_width;
                        i_data_buffer[1] = i_tile_height;
                        index = 2;
                        
                        // convert a single tile
                        for (j = 0; j < i_tile_height; j++) {
                            offset = j * i_width;
                            for (k = 0; k < i_tile_width; k++) {
                                i_data_buffer[index++] = i_data[k + x_offset + y_offset + offset];
                            }
                        }
                        
                        if (g_compression) {
                            uint8_t *c_data = compress_image(i_data_buffer, &i_size_total, g_compression);
                            format->print_compressed_tile(i_output, i_name, curr_tile, i_size_total);
                            output_compressed_array(format, i_output, c_data, i_size_total);
                            free(c_data);
                            
                            // log the compressed size
                            lof("\n %s_tile_%u_compressed (%u -> %d bytes)", i_name, curr_tile, i_size_backup, i_size_total);
                            
                            // warn if compression is worse
                            if (i_size_total > i_size_backup) {
                                lof(" #warning!");
                            }
                        } else {
                            format->print_tile(i_output, i_name, curr_tile, i_size_backup, i_tile_width, i_tile_height);
                            output_array(format, i_output, &i_data_buffer[2], i_tile_width, i_tile_height);
                        }
                        
                        // move to the correct data location
                        if ((x_offset += i_tile_width) > i_width - 1) {
                            x_offset = 0;
                            y_offset += i_width * i_tile_height;
                        }
                    }

                    // build the tilemap table
                    if (g_make_tilemap_ptrs) {
                        format->print_tile_ptrs(i_output, i_name, i_num_tiles, g_compression);
                    }
            // not a tilemap
                } else {
                    // store the width and height as well
                    i_data_buffer[0] = i_width;
                    i_data_buffer[1] = i_height;
                    i_size = i_width * i_height;
                    i_size_total = i_size + 2;
                    
                    // handle bpp mode here
                    if (i_bpp == 8) {
                        memcpy(&i_data_buffer[2], i_data, i_size);
                    } else {
                        force_image_bpp(i_bpp, i_rgba, i_data, &i_data_buffer[2], &i_width, i_height, &i_size);
                        i_size_total = i_size + 2;
                    }
                    
                    // output the image
                    if (g_style_tp) {
                        i_size = group_style_transparent_output(i_data, &i_data_buffer[2], i_width, i_height, i_tindex);
                        i_size_total = i_size + 2;
                    }
                    
                    if (g_compression) {
                        uint8_t *c_data = compress_image(i_data_buffer, &i_size_total, g_compression);
                        format->print_compressed_image(i_output, i_bpp, i_name, i_size_total);
                        output_compressed_array(format, i_output, c_data, i_size_total);
                        free(c_data);
                    } else {
                        format->print_image(i_output, i_bpp, i_name, i_size_total, i_width, i_height);
                        if (g_style_tp) {
                            output_compressed_array(format, i_output, &i_data_buffer[2], i_size);
                        } else {
                            output_array(format, i_output, &i_data_buffer[2], i_width, i_height);
                        }
                    }
                }
                
                if (g_convert_to_tiles) {
                    format->print_tiles_header(g_output, i_name, i_num_tiles, g_compression);

                    if (g_make_tilemap_ptrs) {
                        format->print_tiles_ptrs_header(g_output, i_name, i_num_tiles, g_compression);
                    }
                } else {
                    if (g_style_tp) {
                        format->print_transparent_image_header(g_output, i_name, i_size_total, g_compression);
                    } else {
                        format->print_image_header(g_output, i_name, i_size_total, g_compression);
                    }
                }
                
                // newline for next image
                lof("\n");
                
                // close the outputs
                if (!g_mode_ice) {
                    format->close_output(i_output, OUTPUT_SOURCE);
                    free(i_output);
                }
                
                // free the opened image
                free(i_data_buffer);
                free(i_curr);
                free(i_rgba);
                free(i_data);
                free(i_name);
                free(i_in_name);
                if (i_mapped) { liq_result_destroy(i_mapped);  }
                if (i_image)  { liq_image_destroy(i_image); }
                if (i_attr)   { liq_attr_destroy(i_attr);   }
            }
            
            if (g_out_pal_arr) {
                format->print_palette_header(g_output, g_name, g_pal_len);
            }
            format->print_end_header(g_output);
            format->close_output(g_output, OUTPUT_HEADER);
            format->close_output(g_output, OUTPUT_SOURCE);
            free(g_output);
        }
        
        // free *everything*
        free(curr->image);
        free(g_name);
        free(g_outc_name);
        free(g_outh_name);
        free(g_pal_name);
        if (attr)  { liq_attr_destroy(attr);   }
        if (image) { liq_image_destroy(image); }
        lof("\n");
    }
    
    // say how long it took and if changes should be made
    lof("Converted in %u s\n\n", (unsigned)(time(NULL)-c1));
    if (convpng.bad_conversion) {
        lof("[warning] image quality might be too low.\nplease try grouping similar images, reducing image colors, \nor selecting a better palette if conversion is not ideal.\n\n");
    }
    lof("Finished!\n");
    fclose(convpng.log);

    return 0;
}
