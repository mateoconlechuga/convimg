#include "appvar.h"
#include "main.h"
#include "misc.h"
#include "logging.h"

#include "libs/libimagequant.h"
#include "libs/zx7.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define m8(x)  ((x)&255)
#define mr8(x) (((x)>>8)&255)
#define m16(x) ((x)&65535)

appvar_t appvar[MAX_APPVARS];

// initialize all the output appvar structs
void init_appvars(void) {
    unsigned int s;
    for (s = 0; s < convpng.numappvars; s++) {
        appvar_t *a = &convpng.appvar[s];
        init_appvar(a);
    }
}

// export all the output appvar structs
// we have to free here because we can't convert appvars
void export_appvars(void) {
    unsigned int t, j;
    unsigned int csize, usize;
    const format_t *format;
    output_t *output;

    // return if no appvars to export
    if (!convpng.numappvars) {
        return;
    }

    // find and add all the images to the appvar's data
    add_appvars_images();

    for (t = 0; t < convpng.numappvars; t++) {
        unsigned int num;
        appvar_t *a = &convpng.appvar[t];
        group_t *g = a->g;
        output = output_create();

        lof("exporting appvar: %s.8xv\n", a->name);

        // choose the correct output mode
        if (a->mode == MODE_C) {
            format = &c_format;
        } else if (a->mode == MODE_ASM) {
            format = &asm_format;
        } else if (a->mode == MODE_ICE) {
            format = &ice_format;
        } else {
            errorf("unknown appvar mode");
            return;
        }

        // open the outputs
        format->open_output(output, g->outh, OUTPUT_HEADER);
        format->open_output(output, g->outc, OUTPUT_SOURCE);

        // get total number of things in appvar
        num = g->numimages + a->numpalettes;

        // write out header information
        format->print_header_header(output, g->name);
        format->print_source_header(output, g->outh);
        if (a->write_init) {
            format->print_appvar_load_function_header(output);
        }
        if (a->write_table) {
            format->print_appvar_array(output, a->name, num);
        }

        // add palette information to the end of the appvar
        if (a->palette) {
            unsigned int i;
            for (i = 0; i < a->numpalettes; i++) {
                uint16_t pal[256];
                unsigned int pal_len;
                if (!a->palette_data[i]) {
                    errorf("missing requested palette '%s'", a->palette[i]);
                }
                pal_len = a->palette_data[i]->count;
                for (j = 0; j < pal_len; j++) {
                    liq_color *e = &a->palette_data[i]->entries[j];
                    pal[j] = rgb1555(e->r, e->g, e->b);
                }
                add_appvar_raw(a, pal, pal_len * 2);
            }
        }

        // write out the images and pallete information
        for (j = 0; j < num; j++) {
            if (j < g->numimages) {
                image_t *i = g->image[j];
                lof(" %s\n", i->name, a->offsets[j]);
                bool i_style_tp = i->style == STYLE_RLET;
                if (a->mode == MODE_ICE) {
                    format->print_appvar_image(output, a->name, a->offsets[j], i->name,
                                               i->create_tilemap_ptrs ? i->numtiles * 3 : 3,
                                               i->compression, i->width, i->height, a->write_table, i_style_tp);
                } else {
                    format->print_appvar_image(output, a->name, a->offsets[j], i->name, j,
                                               i->compression, i->width, i->height, a->write_table, i_style_tp);
                }
                if (i->create_tilemap_ptrs) {
                    format->print_tiles_ptrs_header(output, i->name, i->numtiles, i->compression);
                    format->print_tiles_header(output, i->name, i->numtiles, i->compression, true);
                }
            } else {
                if (a->write_table) {
                    format->print_appvar_palette(output, a->palette[j - g->numimages], a->name, a->offsets[j]);
                }
            }
            if (a->mode != MODE_ICE && a->write_table) {
                format->print_next_array_line(output, true, j + 1 == num);
            }
        }

        // free any included palette
        if (a->palette) {
            unsigned int i;
            unsigned int index = g->numimages;
            for (i = 0; i < a->numpalettes; i++) {
                lof(" %s (palette)\n", a->palette[i]);
                format->print_appvar_palette_header(output, a->palette[i], a->name, index,
                                                    a->offsets[index], a->palette_data[i]->count, a->write_table);
                index++;
                free(a->palette[i]);
                free(a->palette_data[i]);
            }
            free(a->palette);
            free(a->palette_data);
        }

        // write the appvar init code
        if (a->write_init) {
            format->print_appvar_load_function(output, a->name, false, a->compression != COMPRESS_NONE);

            for (j = 0; j < g->numimages; j++) {
                image_t *i = g->image[j];
                if (i->create_tilemap_ptrs) {
                    format->print_appvar_load_function_tilemap(output, a->name, i->name, i->numtiles, j, i->compression);
                }
            }

            format->print_appvar_load_function_end(output, a->compression != COMPRESS_NONE);
        }

        // finish exporting the actual appvar
        export_appvar(a, &usize, &csize);

        // print the exported sizes if compressed
        if (a->compression != COMPRESS_NONE) {
            format->print_appvar_export_size(output, a->name, usize, csize);
        }

        // close the outputs
        format->print_end_header(output);
        format->close_output(output, OUTPUT_HEADER);
        format->close_output(output, OUTPUT_SOURCE);

        // free all the things
        free(output);
        free(a->string);
    }
    lof("\n");
}

// check if an image exists in an appvar and make a list of pointers to the ones it does
bool image_is_in_an_appvar(image_t *image) {
    unsigned int i,s;
    bool ret = false;
    if (!image) {
        return ret;
    }
    for (i = 0; i < convpng.numappvars; i++) {
        appvar_t *a = &convpng.appvar[i];
        for (s = 0; s < a->g->numimages; s++) {
            image_t *c = a->g->image[s];
            if (!strcmp(image->name, c->name)) {
                free(c->in);
                free(c->outc);
                free(c->name);
                free(c);
                a->g->image[s] = image;
                a->g->image[s]->found = true;
                ret = true;
            }
        }
    }
    return ret;
}

// check if the palette is needed in an appvar
bool palette_is_in_an_appvar(const char *pal_name) {
    unsigned int i,j;
    for (i = 0; i < convpng.numappvars; i++) {
        appvar_t *a = &convpng.appvar[i];
        for (j = 0; j < a->numpalettes; j++) {
            if (!strcmp(pal_name, a->palette[j])) {
                return true;
            }
        }
    }
    return false;
}

void add_appvars_palette(const char *pal_name, liq_palette *pal) {
    unsigned int i,j;
    for (i = 0; i < convpng.numappvars; i++) {
        appvar_t *a = &convpng.appvar[i];
        for (j = 0; j < a->numpalettes; j++) {
            if (!strcmp(pal_name, a->palette[j])) {
                a->palette_data[j] = malloc(sizeof(liq_palette));
                memcpy(a->palette_data[j], pal, sizeof(liq_palette));
            }
        }
    }
}

void init_appvar(appvar_t *a) {
    const uint8_t header[] = { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };

    // clear data
    a->output = safe_calloc(0x50000, sizeof(uint8_t));
    memset(a->offsets, 0, MAX_OFFSETS * sizeof(uint16_t));

    // write header bytes
    memcpy(a->output, header, sizeof header);

    // compute storage for image offsets
    a->curr = 0;
    a->offsets[0] = a->start - APPVAR_START;
    a->offset = a->start;
}

void add_appvars_images(void) {
    unsigned int a, s;
    for (a = 0; a < convpng.numappvars; a++) {
        appvar_t *var = &convpng.appvar[a];
        for (s = 0; s < var->g->numimages; s++) {
            image_t *image = var->g->image[s];
            if (!image->found) {
                errorf("could not find '%s' for appvar '%s'", image->name, var->name);
            }
            add_appvar_block(var, &image->block);
        }
    }
}

void add_appvar_raw(appvar_t *a, const void *data, unsigned int size) {
    unsigned int offset = a->offset;

    a->offsets[a->curr + 1] = a->offsets[a->curr] + size;
    a->curr++;

    memcpy(&a->output[offset], data, size);
    a->offset += size;
}

void add_appvar_block(appvar_t *a, const data_t *block) {
    unsigned int offset = a->offset;

    if (!block->data) {
        return;
    }

    a->offsets[a->curr + 1] = a->offsets[a->curr] + block->total_size;
    memcpy(&a->output[offset], block->data, block->total_size);
    a->offset += block->total_size;
    a->curr++;
}

void export_appvar(appvar_t *a, unsigned int *usize, unsigned int *csize) {
    uint8_t len_high;
    uint8_t len_low;
    unsigned int data_size;
    unsigned int i,checksum;
    FILE *out_file;

    // gather structure information
    uint8_t *output = a->output;
    unsigned int offset = a->offset;

    if (usize != NULL) {
        *usize = (unsigned int)(offset - a->start);
    }
    if (csize != NULL) {
        *csize = *usize;
    }
    if (a->compression == COMPRESS_ZX7) {
        long delta;
        size_t s_size = offset - a->start;
        unsigned int prev_size = (unsigned int)s_size;
        Optimal *opt = optimize(&output[a->start], s_size);
        uint8_t *ret = compress(opt, &output[a->start], s_size, &s_size, &delta);
        memcpy(&output[a->start], ret, s_size);
        offset = s_size + a->start;
        free(opt);
        free(ret);
        lof("compression: (%u > %u bytes)\n", prev_size, s_size);
        if (csize != NULL) {
            *csize = s_size;
        }
    }

    if (offset > 0xFFE0) { errorf("too much data to output appvar '%s'", a->name); }

    // write name
    memcpy(&output[0x3C], a->name, strlen(a->name));

    // write config bytes
    output[0x37] = 0x0D;
    output[0x3B] = 0x15;
    output[0x45] = 128;

    data_size = offset - 0x37;
    len_high = mr8(data_size);
    len_low = m8(data_size);
    output[0x35] = len_low;
    output[0x36] = len_high;

    data_size = offset - 0x4A;
    len_high = mr8(data_size);
    len_low = m8(data_size);
    output[0x48] = len_low;
    output[0x49] = len_high;

    // write header
    if (a->string) {
        memcpy(&output[0x4A], a->string, strlen(a->string));
    }

    // size bytes
    data_size += 2;
    len_high = mr8(data_size);
    len_low = m8(data_size);
    output[0x39] = len_low;
    output[0x3A] = len_high;
    output[0x46] = len_low;
    output[0x47] = len_high;

    // calculate checksum
    checksum = 0;
    for (i=0x37; i<offset; ++i) {
        checksum = m16(checksum + output[i]);
    }

    output[offset++] = m8(checksum);
    output[offset++] = mr8(checksum);

    // write the buffer to the file
    char *name = str_dupcatdir(a->name, ".8xv");

    if (!(out_file = fopen(name, "wb"))) {
        errorf("unable to open output appvar file.");
    }

    fwrite(output, 1, offset, out_file);

    // close the file
    fclose(out_file);

    // free the memory
    free(name);
    free(output);
}
