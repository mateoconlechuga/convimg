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

unsigned image_is_in_an_appvar(const char *image_name) {
    unsigned i,j;
    unsigned num = 0;
    for (i = 0; i < convpng.numappvars; i++) {
        image_t **image = appvar[i].g->image;
        for (j = 0; j < appvar[i].g->numimages; j++) {
            if (!strcmp(image_name, image[j]->name)) {
                appvar_ptrs[num++] = &appvar[i];
            }
        }
    }
    return num;
}

void output_appvar_init(appvar_t *a, int num_images) {
    static const uint8_t header[] = { 0x2A,0x2A,0x54,0x49,0x38,0x33,0x46,0x2A,0x1A,0x0A };
    
    // clear data
    a->output = safe_calloc(0x10100, sizeof(uint8_t));
    memset(a->offsets, 0, MAX_OFFSETS);
    
    // write header bytes
    memcpy(a->output, header, sizeof header);
    
    // compute storage for image offsets
    a->offset = 0x4A + num_images * sizeof(uint16_t);
    a->offsets[0] = num_images * sizeof(uint16_t);
    a->max_images = num_images;
    a->curr_image = 0;
}

void add_appvars_data(const unsigned i, const uint8_t *data, const size_t size) {
    unsigned j;
    for (j = 0; j < i; j++) {
        add_appvar_data(appvar_ptrs[j], data, size);
    }
}

void add_appvar_data(appvar_t *a, const uint8_t *data, const size_t size) {
    unsigned offset = a->offset;
    unsigned curr   = a->curr_image;
    
    if (offset > 0xFFF0) { errorf("too many images in group to output appvar."); }
    if (curr > a->max_images) { errorf("tried to add too many images to appvar."); }
    
    a->offsets[curr+1] = a->offsets[curr] + size;
    memcpy(a->output + offset, data, size);
    a->offset += size;
    a->curr_image++;
}

void output_appvar_complete(appvar_t *a) {
    uint8_t len_high;
    uint8_t len_low;
    unsigned data_size;
    unsigned i,checksum;
    FILE *out_file;
    
    // gather structure information
    uint8_t *output = a->output;
    unsigned offset = a->offset;
    
    // write name
    lof("exporting appvar: %s.8xp\n", a->name);
    memcpy(&output[0x3C], a->name, strlen(a->name));
    
    // write the offsets to the data structures
    memcpy(&output[0x4A], a->offsets, a->max_images * sizeof(uint16_t));
    
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
