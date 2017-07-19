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
    unsigned int s,g,j,k;
    time_t c1 = time(NULL);
    
    // print out version information
    lof("ConvPNG %d.%d by M.Waltz\n\n", VERSION_MAJOR, VERSION_MINOR);
    
    // init the system
    init_convpng(argc, argv);
    
    // parse the input file
    parse_convpng_ini();

    // convert all the groups
    for (g = 0; g < convpng.numgroups; g++) {
        // init some vars
        liq_palette pal;
        liq_image *image = NULL;
        liq_attr *attr = NULL;
        const format_t *format = NULL;
        output_t *g_output = NULL;
        double diff;
        
        // get current group pointer
        group_t   *curr                = &group[g];
        unsigned   g_mode              = curr->mode;
        
        // already inited structure elements
        uint8_t    g_bpp               = curr->bpp;
        unsigned   g_tindex            = curr->tindex;
        unsigned   g_numimages         = curr->numimages;
        unsigned   g_tile_width        = curr->tile_width;
        unsigned   g_tile_height       = curr->tile_height;
        unsigned   g_style             = curr->style;
        bool       g_pal_fixed_len     = curr->palette_fixed_length;
        bool       g_use_tindex        = curr->use_tindex;
        bool       g_use_omit_color    = curr->use_ocolor;
        bool       g_valid_tcolor      = curr->use_tcolor;
        bool       g_is_global_pal     = curr->is_global_palette;
        bool       g_out_pal_img       = curr->output_palette_image;
        bool       g_out_pal_arr       = curr->output_palette_array;
        char      *g_pal_name          = curr->palette;
        char      *g_name              = curr->name;
        char      *g_outc_name         = curr->outc;
        char      *g_outh_name         = curr->outh;
        unsigned   g_pal_len           = curr->palette_length;
        unsigned   g_fixed_num         = curr->num_fixed_colors;
        fixed_t   *g_fixed             = curr->fixed;
        liq_color  g_transparentcolor  = curr->tcolor;
        liq_color  g_omitcolor         = curr->ocolor;
        
        // init new elements
        bool       g_is_16_bpp         = g_bpp == 16;
        bool       g_mode_c            = g_mode == MODE_C;
        bool       g_mode_asm          = g_mode == MODE_ASM;
        bool       g_mode_ice          = g_mode == MODE_ICE;
        bool       g_mode_appvar       = g_mode == MODE_APPVAR;
        bool       g_use_tcolor        = g_valid_tcolor || g_use_tindex;
        bool       g_style_tp          = g_style == STYLE_RLET;
        bool       g_exported_palette  = false;
        unsigned   g_omitcolor_index;
        
        // determine the output format
        if (g_mode_c) {
            format = &c_format;
        } else
        if (g_mode_asm) {
            format = &asm_format;
        } else
        if (g_mode_ice) {
            format = &ice_format;
        } else
        if (g_mode_appvar) {
            continue;
        }
        
        // let's check if the palette for this group needs to be exported to an appvar
        g_exported_palette = palette_is_in_an_appvar(g_name);
        
        // create the output pointer
        g_output = output_create();
        
        if (g_style_tp && g_bpp != 8) {
            errorf("unsupported bpp for current style");
        }
        
        // force inputs if 16 bpp image group
        if (g_is_16_bpp) {
            free(g_pal_name);
            g_pal_name        = NULL;
            g_is_global_pal   = false;
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
        if (!g_pal_fixed_len) {
            g_pal_len = pow(2, g_bpp);
        }
        
        // set the maximum colors of the palette
        liq_set_max_colors(attr, g_pal_len);
        
        /* check if we need a custom palette */
        if (g_pal_name) {
            uint8_t *pal_arr = NULL;
            uint8_t *custom_pal = NULL;
            
            // use xlibc palette
            if (!strcmp(g_pal_name, "xlibc")) {
                pal_arr = xlibc_palette;
                lof("Using built-in xlibc palette ...\n");
            } else
                
            // use 332 palette
            if (!strcmp(g_pal_name, "rgb332")) {
                pal_arr = rgb332_palette;
                lof("Using built-in rgb332 palette ...\n");
                
            // must be some user specified palette
            } else {
                // some variables
                unsigned int pal_height;
                unsigned int pal_width;
                
                // decode the palette
                unsigned int error = lodepng_decode32_file(&custom_pal, &pal_width, &pal_height, g_pal_name);
                if (error) { errorf("decoding palette %s", pal_width); }
                if (pal_height > 1 || pal_width > 256 || !pal_width) { errorf("palette not formatted correctly."); }
                
                // we should use the custom palette
                pal_arr = custom_pal;
                if (!g_pal_fixed_len) {
                    g_pal_len = pal_width;
                } else if (pal_width < g_pal_len) {
                    g_pal_len = pal_width;
                }
                
                // tell the user what they are doing
                lof("Using defined palette %s ...\n", g_pal_name);
            }
            
            // store the custom palette to the main palette
            pal.count = g_pal_len;
            
            // loop though all the colors
            for (unsigned int h = 0; h < g_pal_len; ++h) {
                unsigned int o = h * 4;
                liq_color c;
                c.r = pal_arr[o + 0];
                c.g = pal_arr[o + 1];
                c.b = pal_arr[o + 2];
                c.a = pal_arr[o + 3];
                pal.entries[h] = c;
            }
            
            // if we allocated a custom palette free it
            free(custom_pal);
            
            if (g_valid_tcolor) {
                lof("warning: ignoring transparent color value for a fixed palette.");
            }
            
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
                if (error) { errorf("decoding %s for palette", pal_in_name); }
                
                pal_image = liq_image_create_rgba(attr, pal_rgba, pal_width, pal_height, 0);
                liq_histogram_add_image(hist, attr, pal_image);
                liq_image_destroy(pal_image);
                
                // free the opened image
                free(pal_rgba);
            }
            
            // add transparent color if neeeded
            if (g_use_tcolor) {
                liq_add_fixed_histogram_color(hist, g_transparentcolor);
            }
            
            // add any fixed palette colors if needed
            if (g_fixed_num) {
                for (j = 0; j < g_fixed_num; j++) {
                    liq_add_fixed_histogram_color(hist, g_fixed[j].color);
                }
            }
            
            liq_error err = liq_histogram_quantize(hist, attr, &res);
            if (err != LIQ_OK) { errorf("generating quantized palette"); }
            
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
                force_color_index(&g_transparentcolor, &pal, &g_pal_len, g_pal_fixed_len, g_tindex);
            }
            
            // find any fixed colors and move to proper indexes
            if (g_fixed_num) {
                for (s = 0; s < g_fixed_num; s++) {
                    force_color_index(&g_fixed[s].color, &pal, &g_pal_len, g_pal_fixed_len, g_fixed[s].index);
                }
            }
            
            // find which index is used for the omit color
            if (g_use_omit_color) {
                for (j = 0; j < g_pal_len; j++) {
                    if (!memcmp(&g_omitcolor, &pal.entries[j], sizeof(liq_color)))
                        break;
                }
                g_omitcolor_index = j;
            }
            
            // free the histogram and resultants
            if (res)  { liq_result_destroy(res);     }
            if (hist) { liq_histogram_destroy(hist); }
        } else {
            lof("16 bpp mode detected, no palette needed ...\n");
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
            format->close_output(g_output, OUTPUT_HEADER);
            format->close_output(g_output, OUTPUT_SOURCE);
            free(g_output);
        } else {
            format->print_source_header(g_output, strip_path(g_outh_name));
            format->print_header_header(g_output, strip_path(g_name));

            // export the palette information to file or appvar
            if (g_exported_palette) {
                add_appvars_palette(g_name, &pal);
            } else if (g_out_pal_arr) {
                format->print_palette(g_output, strip_path(g_name), &pal, g_pal_len);
            }
            
            // log transparent color things
            if (g_use_tcolor) {
                format->print_transparent_index(g_output, strip_path(g_name), g_tindex);
                lof("Transparent Color Index : %u\n", g_tindex);
                lof("Transparent Color : 0x%04X\n", rgb1555(g_transparentcolor.r,
                                                            g_transparentcolor.g,
                                                            g_transparentcolor.b));
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
                uint8_t      *i_data_buffer = NULL;
                
                liq_image    *i_image       = NULL;
                liq_result   *i_mapped      = NULL;
                liq_attr     *i_attr        = liq_attr_create();

                unsigned int  i_size_total  = 0;
                unsigned int  i_size_backup = 0;
                
                // init the things for each image
                char         *i_source_name = i_curr->outc;
                char         *i_in_name     = i_curr->in;
                char         *i_name        = i_curr->name;
                uint8_t       i_bpp         = g_bpp;
                uint8_t       i_tindex      = g_tindex;
                unsigned int  i_compression = i_curr->compression;
                unsigned int  i_style       = i_curr->style;
                unsigned int  i_tile_width  = g_tile_width;
                unsigned int  i_tile_height = g_tile_height;
                unsigned int  i_num_tiles   = 0;
                unsigned int  i_width;
                unsigned int  i_height;
                unsigned int  i_size;
                unsigned int  i_error;
                
                // determine if image needs to be exported to an appvar
                bool i_appvar = image_is_in_an_appvar(i_curr);
                bool i_style_rlet = i_style == STYLE_RLET;
                
                // tilemap things
                bool i_convert_to_tilemap = i_curr->convert_to_tilemap;
                bool i_create_tilemap_ptrs = i_curr->create_tilemap_ptrs;
                
                // open the file and make a rgba array
                i_error = lodepng_decode32_file(&i_rgba, &i_width, &i_height, i_in_name);
                if (i_error == LODEPNG_ERR_OPEN) { errorf("could not open '%s'", i_in_name); }
                if (i_error) { errorf("decoding '%s'", i_in_name); }

                // get the actual size of the image
                i_size = i_width * i_height;

                // quick tilemap check
                if (i_convert_to_tilemap) {
                    i_curr->numtiles = i_num_tiles = (i_width / g_tile_width) * (i_height / g_tile_height);

                    if ((i_width % g_tile_width) || (i_height % g_tile_height)) {
                        errorf("image dimensions do not align to tile width and/or height value");
                    }
                }

                // allocate and decode the image
                i_data = safe_malloc(i_size + 2);

                if (!g_is_16_bpp) {
                    liq_set_max_colors(i_attr, g_pal_len);
                    i_image = liq_image_create_rgba(i_attr, i_rgba, i_width, i_height, 0);
                    if (!i_image) { errorf("could not create image"); }

                    // add all the palette colors
                    for (j = 0; j < g_pal_len; j++) { liq_image_add_fixed_color(i_image, pal.entries[j]); }

                    // quantize image against palette
                    if (!(i_mapped = liq_quantize_image(i_attr, i_image))) {errorf("could not quantize image"); }
                    liq_write_remapped_image(i_mapped, i_image, i_data, i_size);

                    // if custom palette, hard to compute accuratly
                    if (g_pal_name) {
                        lof(" %s : converted!", i_name);
                        convpng.bad_conversion = true;
                    } else {
                        if ((diff = liq_get_remapping_error(i_mapped)) > 15) { convpng.bad_conversion = true; }
                        lof(" %s : %.2f%%", i_name, 100-diff > 0 ? 100-diff : 0);
                    }
                } else {
                    lof(" %s : converted!", i_name);
                }

                // open the outputs
                output_t *i_output;

                if (!g_mode_ice) {
                    i_output = output_create();
                    format->open_output(i_output, i_source_name, OUTPUT_SOURCE);
                } else {
                    i_output = g_output;
                }

                // write all the image data to the ouputs
                format->print_image_source_header(i_output, strip_path(g_outh_name));
                
                // allocate a buffer for storing the new data
                i_data_buffer = safe_malloc(i_width * i_height * 2 + 2);
                
                if (i_convert_to_tilemap) {
                    unsigned int tile_num = 0;
                    unsigned int x_offset = 0;
                    unsigned int y_offset = 0;
                    unsigned int offset_size = 0;
                    unsigned int offset;
                    unsigned int *offsets;
                    unsigned int index;

                    // disable output to offset stack
                    if (i_appvar) { add_appvars_offsets_state(false); }
                    
                    offsets = malloc(i_num_tiles * sizeof(unsigned int));
                    
                    for (; tile_num < i_num_tiles; tile_num++) {
                        i_data_buffer[0] = i_tile_width;
                        i_data_buffer[1] = i_tile_height;
                        index = SIZE_BYTES;
                        i_size = i_tile_width * i_tile_height;
                        i_size_total = i_size + SIZE_BYTES;
                        i_size_backup = i_size_total;
                        
                        // convert a single tile
                        for (j = 0; j < i_tile_height; j++) {
                            offset = j * i_width;
                            for (k = 0; k < i_tile_width; k++) {
                                i_data_buffer[index++] = i_data[k + x_offset + y_offset + offset];
                            }
                        }

                        if (g_use_omit_color) {
                            i_size = remove_elements(&i_data_buffer[SIZE_BYTES], i_size, g_omitcolor_index);
                            i_size_total = i_size + SIZE_BYTES;
                        }
                        
                        if (i_compression) {
                            uint8_t *c_data = compress_image(i_data_buffer, &i_size_total, i_compression);
                            if (i_appvar) {
                                add_appvars_data(c_data, i_size_total);
                            } else {
                                format->print_compressed_tile(i_output, i_name, tile_num, i_size_total);
                                output_array_compressed(format, i_output, c_data, i_size_total);
                            }
                            free(c_data);

                            // log the compressed size
                            lof("\n %s_tile_%u_compressed (%u > %d bytes)", i_name, tile_num, i_size_backup, i_size_total);

                            // warn if compression is worse
                            if (i_size_total > i_size_backup) {
                                lof(" #warning!");
                            }
                        } else {
                            if (i_appvar) {
                                add_appvars_data(i_data_buffer, i_size_total);
                            } else {
                                format->print_tile(i_output, i_name, tile_num, i_size_total, i_tile_width, i_tile_height);
                                if (g_use_omit_color) {
                                    output_array_compressed(format, i_output, &i_data_buffer[SIZE_BYTES], i_size);
                                } else {
                                    output_array(format, i_output, &i_data_buffer[SIZE_BYTES], i_tile_width, i_tile_height);
                                }
                            }
                        }

                        // store the size
                        offset_size += i_size_total;
                        offsets[tile_num] = offset_size;
                        
                        // move to the correct data location
                        if ((x_offset += i_tile_width) > i_width - 1) {
                            x_offset = 0;
                            y_offset += i_width * i_tile_height;
                        }
                    }

                    // add the image offsets
                    if (i_appvar) {
                        add_appvars_offset(offset_size);
                    }
                    
                    // build the tilemap table
                    if (i_create_tilemap_ptrs) {
                        format->print_tile_ptrs(i_output, i_name, i_num_tiles, i_compression, i_appvar, offsets);
                    }
                    
                    // free all the offsets
                    free(offsets);

                // not a tilemap
                } else {
                    // store the width and height as well
                    i_data_buffer[0] = i_width;
                    i_data_buffer[1] = i_height;
                    i_size = i_width * i_height;
                    i_size_total = i_size + SIZE_BYTES;
                    i_size_backup = i_size_total;
                    
                    // handle bpp mode here
                    if (i_bpp == 8) {
                        memcpy(&i_data_buffer[SIZE_BYTES], i_data, i_size);
                    } else {
                        force_image_bpp(i_bpp, i_rgba, i_data, &i_data_buffer[SIZE_BYTES], &i_width, i_height, &i_size);
                        i_size_total = i_size + SIZE_BYTES;
                    }

                    // do we need to omit a color from output
                    if (g_use_omit_color) {
                        i_size = remove_elements(&i_data_buffer[SIZE_BYTES], i_size, g_omitcolor_index);
                        i_size_total = i_size + SIZE_BYTES;
                    } else
                    
                    // check if rlet style
                    if (i_style_rlet) {
                        i_size = group_rlet_output(i_data, &i_data_buffer[SIZE_BYTES], i_width, i_height, i_tindex);
                        i_size_total = i_size + SIZE_BYTES;
                    }
                    
                    // output the image
                    if (i_compression) {
                        uint8_t *c_data = compress_image(i_data_buffer, &i_size_total, i_compression);
                        if (i_appvar) {
                            add_appvars_data(c_data, i_size_total);
                        } else {
                            format->print_compressed_image(i_output, i_bpp, i_name, i_size_total);
                            output_array_compressed(format, i_output, c_data, i_size_total);
                        }
                        lof(" (%u > %d bytes)", i_size_backup, i_size_total);
                        free(c_data);
                    } else {
                        if (i_appvar) {
                            add_appvars_data(i_data_buffer, i_size_total);
                        } else {
                            format->print_image(i_output, i_bpp, i_name, i_size_total, i_width, i_height);
                            if (i_style_rlet || g_use_omit_color) {
                                output_array_compressed(format, i_output, &i_data_buffer[SIZE_BYTES], i_size);
                            } else {
                                output_array(format, i_output, &i_data_buffer[SIZE_BYTES], i_width, i_height);
                            }
                        }
                    }
                }
                
                if (!i_appvar) {
                    if (i_convert_to_tilemap) {
                        format->print_tiles_header(g_output, i_name, i_num_tiles, i_compression, i_appvar);

                        if (i_create_tilemap_ptrs) {
                            format->print_tiles_ptrs_header(g_output, i_name, i_num_tiles, i_compression);
                        }
                    } else if (!i_appvar) {
                        if (g_style_tp) {
                            format->print_transparent_image_header(g_output, i_name, i_size_total, i_width, i_height, i_compression);
                        } else {
                            format->print_image_header(g_output, i_name, i_size_total, i_width, i_height, i_compression);
                        }
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
                free(i_rgba);
                free(i_data);
                if (i_mapped) { liq_result_destroy(i_mapped);  }
                if (i_image)  { liq_image_destroy(i_image); }
                if (i_attr)   { liq_attr_destroy(i_attr);   }
            }
            
            if (!g_exported_palette && g_out_pal_arr) {
                format->print_palette_header(g_output, strip_path(g_name), g_pal_len);
            }
            format->print_end_header(g_output);
            format->close_output(g_output, OUTPUT_HEADER);
            format->close_output(g_output, OUTPUT_SOURCE);
            free(g_output);
        }
        
        // free *everything*
        if (attr)  { liq_attr_destroy(attr);   }
        if (image) { liq_image_destroy(image); }
        lof("\n");
    }
    
    // export final appvar data
    export_appvars();
    
    // free everything else
    for (g = 0; g < convpng.numgroups; g++) {
        group_t *curr = &group[g];
        if (curr->mode != MODE_APPVAR) {
            for (s = 0; s < curr->numimages; s++) {
                image_t *i_curr = curr->image[s];
                if (i_curr) {
                    free(i_curr->name);
                    free(i_curr->outc);
                    free(i_curr->in);
                    free(i_curr);
                }
            }
        }
        free(curr->palette);
        free(curr->image);
        free(curr->name);
        free(curr->outc);
        free(curr->outh);
        curr->image = NULL;
    }
    
    // say how long it took and if changes should be made
    lof("Converted in %u s\n\n", (unsigned)(time(NULL)-c1));
    if (convpng.bad_conversion) {
        lof("[warning] image quality might be too low.\nplease try grouping similar images, reducing image colors, \nor selecting a better palette if conversion is not ideal.\n\n");
    }
    lof("Finished!\n");
    
    return cleanup_convpng();
}
