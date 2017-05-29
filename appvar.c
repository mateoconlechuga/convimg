#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "misc.h"
#include "appvar.h"
#include "logging.h"

#define m8(x)  ((x)&255)
#define mr8(x) (((x)>>8)&255)
#define m16(x) ((x)&65535)

appvar_t appvar[MAX_APPVARS];
appvar_t *appvar_ptrs[MAX_APPVARS];
static unsigned int image_num_appvars = 0;

// initialize all the output appvar structs
void init_appvars(void) {
    unsigned int a;
    for (a = 0; a < convpng.numappvars; a++) {
        output_appvar_init(&appvar[a], appvar[a].g->numimages);
    }
}

// export all the output appvar structs
// we have to free here because we can't convert appvars
void export_appvars(void) {
    unsigned int t, j;
    const format_t *format;
    output_t *output;
    
    // return if no appvars to export
    if (!convpng.numappvars) {
        return;
    }
    
    for (t = 0; t < convpng.numappvars; t++) {
        appvar_t *a = &appvar[t];
        group_t *g = a->g;
        output_appvar_complete(a);
        output = output_create();
            
        // choose the correct output mode
        if (a->mode == MODE_C) {
            format = &c_format;
        } else {
            errorf("unknown appvar mode");
        }
        
        // open the outputs
        format->open_output(output, g->outh, OUTPUT_HEADER);
        format->open_output(output, g->outc, OUTPUT_SOURCE);
        
        // write out header information
        format->print_header_header(output, g->name);
        format->print_source_header(output, g->outh);
        format->print_appvar_array(output, a->name, g->numimages);
        
        for (j = 0; j < g->numimages; j++) {
            image_t *i = g->image[j];
            bool i_style_tp = i->style == STYLE_TRANSPARENT;
                
            format->print_appvar_image(output, a->name, a->offsets[j], i->name, j, i->compression, i_style_tp);
            format->print_next_array_line(output, j + 1 == g->numimages);
            
            free(i->name);
            free(i->outc);
            free(i->in);
            free(i);
        }
        
        // close the outputs
        format->print_end_header(output);
        format->close_output(output, OUTPUT_HEADER);
        format->close_output(output, OUTPUT_SOURCE);
        
        // free all the things
        free(g->palette_name);
        free(g->image);
        free(g->name);
        free(g->outc);
        free(g->outh);
        free(output);
    }
    lof("\n");
}

// check if an image exists in an avvar and make a list of pointers to the ones it does
bool image_is_in_an_appvar(const char *image_name) {
    unsigned int i,j;
    image_num_appvars = 0;
    for (i = 0; i < convpng.numappvars; i++) {
        image_t **image = appvar[i].g->image;
        for (j = 0; j < appvar[i].g->numimages; j++) {
            if (!strcmp(image_name, image[j]->name)) {
                appvar_ptrs[image_num_appvars++] = &appvar[i];
            }
        }
    }
    return image_num_appvars != 0;
}

void output_appvar_init(appvar_t *a, int num_images) {
    const uint8_t header[] = { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };
    
    // clear data
    a->output = safe_calloc(0x10100, sizeof(uint8_t));
    memset(a->offsets, 0, MAX_OFFSETS);
    
    // write header bytes
    memcpy(a->output, header, sizeof header);
    
    // compute storage for image offsets
    a->offsets[0] = 0;
    a->max_images = num_images;
    a->curr_image = 0;
    a->offset = 0x4A;
}

void add_appvars_data(const uint8_t *data, const size_t size) {
    unsigned int j;
    for (j = 0; j < image_num_appvars; j++) {
        add_appvar_data(appvar_ptrs[j], data, size);
    }
}

void add_appvar_data(appvar_t *a, const uint8_t *data, const size_t size) {
    unsigned int offset = a->offset;
    unsigned int curr   = a->curr_image;
    
    if (offset > 0xFFF0)      { errorf("too many images in group to output appvar"); }
    if (curr > a->max_images) { errorf("tried to add too many images to appvar"); }
    
    a->offsets[curr+1] = a->offsets[curr] + size;
    memcpy(a->output + offset, data, size);
    a->offset += size;
    a->curr_image++;
}

void output_appvar_complete(appvar_t *a) {
    uint8_t len_high;
    uint8_t len_low;
    unsigned int data_size;
    unsigned int i,checksum;
    FILE *out_file;
    
    // gather structure information
    uint8_t *output = a->output;
    unsigned int offset = a->offset;
    
    // write name
    lof("exporting appvar: %s.8xp\n", a->name);
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
    char name[14];
    strcpy(name, a->name);
    strcat(name, ".8xv");
    
    if (!(out_file = fopen(name, "wb"))) {
        errorf("unable to open output appvar file.");
    }
    
    fwrite(output, 1, offset, out_file);
    
    // close the file
    fclose(out_file);
    
    // free the memory
    free(output);
}
